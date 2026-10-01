#include "voice_party.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMediaDevices>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QtEndian>
#include <rtc/rtc.hpp>
#include <opus.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <functional>
#include <unordered_map>
#include <utility>

namespace {
constexpr quint16 DiscoveryPort = 46579;
constexpr quint16 DefaultPartyPort = 46580;
constexpr quint16 IcePortBegin = 46581;
constexpr quint16 IcePortEnd = 46600;
constexpr int FrameSamples = 960; // 20 ms at 48 kHz
constexpr int FrameBytes = FrameSamples * sizeof(opus_int16);
constexpr int MaxPeers = 8;
constexpr int MaxSignalBytes = 64 * 1024;
constexpr int MaxVoiceBuffer = 32 * 1024;

QString safeName(QString name) {
    name = name.trimmed().left(24);
    return name.isEmpty() ? QObject::tr("Jugador") : name;
}
QString errorText(const std::exception& error) { return QString::fromUtf8(error.what()); }

template<typename F>
void postToParty(const std::weak_ptr<VoiceParty::CallbackTarget>& weak, F&& callback) {
    if (auto target = weak.lock()) {
        const std::lock_guard lock(target->mutex);
        if (target->instance)
            QMetaObject::invokeMethod(target->instance,
                                      [weak, callback = std::forward<F>(callback)]() mutable {
                if (auto current = weak.lock()) {
                    VoiceParty* instance = nullptr;
                    {
                        const std::lock_guard currentLock(current->mutex);
                        instance = current->instance;
                    }
                    if (instance) callback(instance);
                }
            }, Qt::QueuedConnection);
    }
}
}

struct VoiceParty::Peer {
    QPointer<QTcpSocket> socket;
    QByteArray incoming;
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::DataChannel> chat;
    std::shared_ptr<rtc::DataChannel> voice;
    QList<QPair<QString, QString>> pendingCandidates;
    QString name;
    QString targetAddress;
    quint32 id = 0;
    bool remoteReady = false;
};

struct VoiceParty::Audio {
    using Encoder = std::unique_ptr<OpusEncoder, decltype(&opus_encoder_destroy)>;
    using Decoder = std::unique_ptr<OpusDecoder, decltype(&opus_decoder_destroy)>;
    Encoder encoder{nullptr, &opus_encoder_destroy};
    std::unordered_map<quint32, Decoder> decoders;
    std::unordered_map<quint32, std::deque<std::array<opus_int16, FrameSamples>>> frames;
    std::unique_ptr<QAudioSource> input;
    std::unique_ptr<QAudioSink> output;
    QIODevice* capture = nullptr;
    QIODevice* playback = nullptr;
    QByteArray pending;
};

VoiceParty::VoiceParty(QObject* parent) : QObject(parent), callbacks_(std::make_shared<CallbackTarget>()) {
    callbacks_->instance = this;
    status_ = tr("Crea una sala o únete a la de un amigo. Sin servidor externo.");
    connect(&server_, &QTcpServer::newConnection, this, &VoiceParty::acceptPeer);
    discovery_.bind(QHostAddress::AnyIPv4, DiscoveryPort,
                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(&discovery_, &QUdpSocket::readyRead, this, &VoiceParty::readDiscovery);
    heartbeat_.setInterval(2000);
    connect(&heartbeat_, &QTimer::timeout, this, [this] { announce(); updateNearby(); });
    heartbeat_.start();
    playback_.setInterval(20);
    connect(&playback_, &QTimer::timeout, this, &VoiceParty::playAudio);
}

VoiceParty::~VoiceParty() {
    {
        const std::lock_guard lock(callbacks_->mutex);
        callbacks_->instance = nullptr;
    }
    leave();
}

void VoiceParty::setStatus(const QString& text) { status_ = text; emit changed(); }

QString VoiceParty::localAddresses() const {
    QStringList addresses;
    for (const auto& interface : QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsUp) ||
            (interface.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto& entry : interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                addresses << entry.ip().toString();
        }
    }
    addresses.removeDuplicates();
    return addresses.isEmpty() ? QStringLiteral("127.0.0.1") : addresses.join(", ");
}

void VoiceParty::setMuted(bool muted) {
    if (muted_ == muted) return;
    muted_ = muted;
    emit changed();
}

bool VoiceParty::host(const QString& name, int requestedPort) {
    if (requestedPort < 0 || requestedPort > 65535) {
        setStatus(tr("El puerto debe estar entre 1 y 65535.")); return false;
    }
    leave();
    if (!server_.listen(QHostAddress::AnyIPv4, static_cast<quint16>(requestedPort))) {
        setStatus(tr("No se pudo abrir la sala: %1").arg(server_.errorString()));
        return false;
    }
    port_ = server_.serverPort();
    displayName_ = safeName(name);
    roomCode_ = QString::number(QRandomGenerator::system()->bounded(100000000)).rightJustified(8, '0');
    hosting_ = active_ = true;
    selfId_ = 1;
    nextId_ = 2;
    messages_.clear();
    publishRoster();
    startAudio();
    setStatus(tr("Sala abierta. Comparte la IP, el puerto y el código con tus amigos."));
    announce();
    return true;
}

void VoiceParty::join(const QString& address, int requestedPort, const QString& code, const QString& name) {
    const auto hostName = address.trimmed();
    if (hostName.isEmpty() || hostName.size() > 255 || requestedPort <= 0 || requestedPort > 65535 ||
        code.trimmed().size() != 8) {
        setStatus(tr("Escribe la IP, el puerto y el código de 8 cifras de la sala."));
        return;
    }
    leave();
    hosting_ = false;
    active_ = true;
    port_ = static_cast<quint16>(requestedPort);
    displayName_ = safeName(name);
    roomCode_ = code.trimmed();
    remoteAddress_ = hostName;
    messages_.clear();
    auto* socket = new QTcpSocket(this);
    auto peer = std::make_shared<Peer>();
    peer->socket = socket;
    peer->name = tr("Anfitrión");
    peers_.insert(socket, peer);
    attachSocket(socket, peer);
    connect(socket, &QTcpSocket::connected, this, [this, socket] {
        sendSignal(socket, {{"type", "hello"}, {"name", displayName_}, {"code", roomCode_},
                            {"targetAddress", socket->peerAddress().toString()}});
    });
    socket->connectToHost(hostName, port_);
    setStatus(tr("Conectando con %1:%2…").arg(hostName).arg(port_));
}

void VoiceParty::leave() {
    active_ = false;
    playback_.stop();
    stopAudio();
    server_.close();
    for (auto it = peers_.begin(); it != peers_.end(); ++it) {
        const auto& peer = it.value();
        if (peer->chat) { peer->chat->resetCallbacks(); peer->chat->close(); }
        if (peer->voice) { peer->voice->resetCallbacks(); peer->voice->close(); }
        if (peer->connection) { peer->connection->resetCallbacks(); peer->connection->close(); }
        if (peer->socket) {
            disconnect(peer->socket, nullptr, this, nullptr);
            peer->socket->disconnectFromHost();
            peer->socket->deleteLater();
        }
    }
    peers_.clear();
    hosting_ = false;
    port_ = 0;
    roomCode_.clear();
    lastConnectionError_.clear();
    participants_.clear();
    selfId_ = 0;
    setStatus(tr("Sala cerrada."));
}

void VoiceParty::acceptPeer() {
    while (server_.hasPendingConnections()) {
        auto* socket = server_.nextPendingConnection();
        if (peers_.size() >= MaxPeers) { socket->disconnectFromHost(); socket->deleteLater(); continue; }
        auto peer = std::make_shared<Peer>();
        peer->socket = socket;
        peers_.insert(socket, peer);
        attachSocket(socket, peer);
    }
}

void VoiceParty::attachSocket(QTcpSocket* socket, const std::shared_ptr<Peer>& peer) {
    connect(socket, &QTcpSocket::readyRead, this, [this, weak = std::weak_ptr(peer)] {
        if (auto current = weak.lock()) readSignal(current);
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
        auto peer = peers_.take(socket);
        if (peer) {
            if (peer->chat) { peer->chat->resetCallbacks(); peer->chat->close(); }
            if (peer->voice) { peer->voice->resetCallbacks(); peer->voice->close(); }
            if (peer->connection) { peer->connection->resetCallbacks(); peer->connection->close(); }
        }
        socket->deleteLater();
        if (!active_) return;
        if (hosting_) { publishRoster(); setStatus(tr("Un jugador salió de la sala.")); }
        else {
            const auto error = lastConnectionError_;
            leave();
            if (!error.isEmpty()) setStatus(error);
        }
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, socket](QAbstractSocket::SocketError) {
        if (active_ && socket->state() != QAbstractSocket::ConnectedState &&
            lastConnectionError_.isEmpty()) {
            lastConnectionError_ = tr("Error de conexión: %1").arg(socket->errorString());
            setStatus(lastConnectionError_);
        }
    });
}

void VoiceParty::sendSignal(QTcpSocket* socket, const QJsonObject& object) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;
    auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaxSignalBytes - 1) return;
    bytes.append('\n');
    socket->write(bytes);
}

void VoiceParty::readSignal(const std::shared_ptr<Peer>& peer) {
    if (!peer->socket) return;
    peer->incoming += peer->socket->readAll();
    if (peer->incoming.size() > MaxSignalBytes) { peer->socket->disconnectFromHost(); return; }
    qsizetype end;
    while ((end = peer->incoming.indexOf('\n')) >= 0) {
        const auto line = peer->incoming.left(end);
        peer->incoming.remove(0, end + 1);
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            peer->socket->disconnectFromHost(); return;
        }
        handleSignal(peer, document.object());
    }
}

void VoiceParty::handleSignal(const std::shared_ptr<Peer>& peer, const QJsonObject& object) {
    const auto type = object.value("type").toString();
    try {
        if (type == "hello" && hosting_ && !peer->connection) {
            if (object.value("code").toString() != roomCode_) {
                sendSignal(peer->socket, {{"type", "error"}, {"message", tr("Código incorrecto.")}});
                peer->socket->disconnectFromHost(); return;
            }
            peer->name = safeName(object.value("name").toString());
            peer->id = nextId_++;
            const QHostAddress target(object.value("targetAddress").toString());
            if (target.protocol() == QAbstractSocket::IPv4Protocol)
                peer->targetAddress = target.toString();
            sendSignal(peer->socket, {{"type", "welcome"}, {"id", static_cast<int>(peer->id)},
                                      {"name", displayName_}});
            configurePeer(peer);
            peer->chat = peer->connection->createDataChannel("chat");
            attachChannel(peer, peer->chat);
            rtc::DataChannelInit voiceInit;
            voiceInit.reliability.unordered = true;
            voiceInit.reliability.maxRetransmits = 0;
            peer->voice = peer->connection->createDataChannel("voice", voiceInit);
            attachChannel(peer, peer->voice);
            peer->connection->setLocalDescription(rtc::Description::Type::Offer);
            publishRoster();
        } else if (type == "welcome" && !hosting_) {
            selfId_ = static_cast<quint32>(object.value("id").toInt());
            peer->name = safeName(object.value("name").toString());
            configurePeer(peer);
        } else if ((type == "offer" || type == "answer") && peer->connection) {
            const auto sdp = object.value("sdp").toString().toUtf8().toStdString();
            if (sdp.size() > MaxSignalBytes - 1024) return;
            peer->connection->setRemoteDescription(rtc::Description(sdp, type.toStdString()));
            peer->remoteReady = true;
            for (const auto& [candidate, mid] : peer->pendingCandidates)
                peer->connection->addRemoteCandidate(rtc::Candidate(candidate.toStdString(), mid.toStdString()));
            peer->pendingCandidates.clear();
            if (type == "offer") peer->connection->setLocalDescription(rtc::Description::Type::Answer);
        } else if (type == "candidate" && peer->connection) {
            const auto candidate = object.value("candidate").toString();
            const auto mid = object.value("mid").toString();
            if (candidate.size() > 512 || mid.size() > 64) return;
            if (peer->remoteReady)
                peer->connection->addRemoteCandidate(rtc::Candidate(candidate.toStdString(), mid.toStdString()));
            else peer->pendingCandidates.append({candidate, mid});
        } else if (type == "roster" && !hosting_) {
            participants_ = object.value("names").toArray().toVariantList();
            emit changed();
        } else if (type == "error" && !hosting_) {
            lastConnectionError_ = object.value("message").toString(tr("El anfitrión rechazó la conexión."));
            setStatus(lastConnectionError_);
        }
    } catch (const std::exception& error) {
        setStatus(tr("No se pudo establecer WebRTC: %1").arg(errorText(error)));
    }
}

void VoiceParty::configurePeer(const std::shared_ptr<Peer>& peer) {
    rtc::Configuration config;
    config.disableAutoNegotiation = true;
    config.portRangeBegin = IcePortBegin;
    config.portRangeEnd = IcePortEnd;
    peer->connection = std::make_shared<rtc::PeerConnection>(config);
    const auto weak = std::weak_ptr(peer);
    const auto callbackTarget = std::weak_ptr(callbacks_);
    peer->connection->onLocalDescription([weak, callbackTarget](rtc::Description description) {
        const auto sdp = QString::fromStdString(description.generateSdp());
        const auto type = QString::fromStdString(description.typeString());
        postToParty(callbackTarget, [weak, sdp, type](VoiceParty* party) {
            if (auto current = weak.lock()) party->sendSignal(current->socket,
                {{"type", type}, {"sdp", sdp}});
        });
    });
    peer->connection->onLocalCandidate([weak, callbackTarget](rtc::Candidate candidate) {
        const auto value = QString::fromStdString(candidate.candidate());
        const auto mid = QString::fromStdString(candidate.mid());
        postToParty(callbackTarget, [weak, value, mid](VoiceParty* party) {
            if (auto current = weak.lock()) {
                party->sendSignal(current->socket,
                    {{"type", "candidate"}, {"candidate", value}, {"mid", mid}});
                // The dialled public IP is supplied by the guest. A forwarded UDP
                // port can then be tried without any STUN or TURN service.
                if (party->hosting_ && !current->targetAddress.isEmpty()) {
                    try {
                        rtc::Candidate publicCandidate(value.toStdString(), mid.toStdString());
                        if (publicCandidate.type() == rtc::Candidate::Type::Host) {
                            publicCandidate.changeAddress(current->targetAddress.toStdString());
                            party->sendSignal(current->socket,
                                {{"type", "candidate"},
                                 {"candidate", QString::fromStdString(publicCandidate.candidate())}, {"mid", mid}});
                        }
                    } catch (const std::exception&) { /* Keep the local candidate. */ }
                }
            }
        });
    });
    peer->connection->onDataChannel([weak, callbackTarget](std::shared_ptr<rtc::DataChannel> channel) {
        postToParty(callbackTarget, [weak, channel = std::move(channel)](VoiceParty* party) {
            if (auto current = weak.lock()) party->attachChannel(current, channel);
        });
    });
    peer->connection->onStateChange([weak, callbackTarget](rtc::PeerConnection::State state) {
        postToParty(callbackTarget, [weak, state](VoiceParty* party) {
            if (!weak.lock() || !party->active_) return;
            if (state == rtc::PeerConnection::State::Connected)
                party->setStatus(party->tr("Voz WebRTC conectada."));
            else if (state == rtc::PeerConnection::State::Failed)
                party->setStatus(party->tr("No se pudo abrir el canal directo. Revisa los puertos UDP del anfitrión."));
        });
    });
}

void VoiceParty::attachChannel(const std::shared_ptr<Peer>& peer,
                               const std::shared_ptr<rtc::DataChannel>& channel) {
    const auto label = channel->label();
    if (label == "chat") peer->chat = channel;
    else if (label == "voice") peer->voice = channel;
    else { channel->close(); return; }
    const auto weak = std::weak_ptr(peer);
    const auto callbackTarget = std::weak_ptr(callbacks_);
    channel->onOpen([weak, callbackTarget] {
        postToParty(callbackTarget, [weak](VoiceParty* party) {
            if (weak.lock() && party->active_) party->setStatus(party->tr("Canal de charla conectado."));
        });
    });
    if (label == "chat") {
        channel->onMessage([weak, callbackTarget](rtc::message_variant message) {
            if (!std::holds_alternative<rtc::string>(message)) return;
            const auto text = QString::fromStdString(std::get<rtc::string>(message));
            postToParty(callbackTarget, [weak, text](VoiceParty* party) {
                if (auto current = weak.lock()) party->handleChat(current, text);
            });
        });
    } else {
        channel->onMessage([weak, callbackTarget](rtc::message_variant message) {
            if (!std::holds_alternative<rtc::binary>(message)) return;
            const auto& bytes = std::get<rtc::binary>(message);
            if (bytes.size() < 5 || bytes.size() > 1280) return;
            const QByteArray packet(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
            postToParty(callbackTarget, [weak, packet](VoiceParty* party) {
                if (auto current = weak.lock()) party->handleVoice(current, packet);
            });
        });
    }
}

void VoiceParty::publishRoster() {
    if (!hosting_) return;
    QJsonArray names;
    names.append(displayName_);
    for (const auto& peer : std::as_const(peers_))
        if (peer->id) names.append(peer->name);
    participants_ = names.toVariantList();
    emit changed();
    for (const auto& peer : std::as_const(peers_))
        sendSignal(peer->socket, {{"type", "roster"}, {"names", names}});
}

void VoiceParty::sendText(const QString& text) {
    const auto trimmed = text.trimmed().left(400);
    if (trimmed.isEmpty() || !active_) return;
    const auto packet = QString::fromUtf8(QJsonDocument(QJsonObject{{"name", displayName_},
                                                            {"text", trimmed}}).toJson(QJsonDocument::Compact));
    bool sent = false;
    for (const auto& peer : std::as_const(peers_)) {
        if (peer->chat && peer->chat->isOpen()) {
            peer->chat->send(packet.toUtf8().toStdString());
            sent = true;
        }
    }
    if (!sent && !hosting_) { setStatus(tr("Todavía no hay un canal de charla conectado.")); return; }
    messages_.append(QVariantMap{{"name", displayName_}, {"text", trimmed}});
    while (messages_.size() > 100) messages_.removeFirst();
    emit changed();
}

void VoiceParty::handleChat(const std::shared_ptr<Peer>& peer, const QString& text) {
    if (text.size() > 1000) return;
    const auto document = QJsonDocument::fromJson(text.toUtf8());
    if (!document.isObject()) return;
    const auto incoming = document.object().value("text").toString().trimmed().left(400);
    if (incoming.isEmpty()) return;
    const auto name = hosting_ ? peer->name : safeName(document.object().value("name").toString());
    messages_.append(QVariantMap{{"name", name}, {"text", incoming}});
    while (messages_.size() > 100) messages_.removeFirst();
    emit changed();
    if (hosting_) {
        const auto relay = QJsonDocument(QJsonObject{{"name", name}, {"text", incoming}})
                               .toJson(QJsonDocument::Compact).toStdString();
        for (const auto& other : std::as_const(peers_))
            if (other != peer && other->chat && other->chat->isOpen()) other->chat->send(relay);
    }
}

void VoiceParty::emitVoice(const QByteArray& opusFrame) {
    if (!active_ || muted_ || opusFrame.isEmpty()) return;
    QByteArray packet(4, '\0');
    qToBigEndian(selfId_, packet.data());
    packet += opusFrame;
    for (const auto& peer : std::as_const(peers_))
        if (peer->voice && peer->voice->isOpen() && peer->voice->bufferedAmount() < MaxVoiceBuffer)
            peer->voice->sendBuffer(packet);
}

void VoiceParty::handleVoice(const std::shared_ptr<Peer>& peer, const QByteArray& data) {
    if (!audio_ || data.size() < 5) return;
    QByteArray packet = data;
    quint32 speaker = qFromBigEndian<quint32>(packet.constData());
    if (hosting_) {
        speaker = peer->id;
        qToBigEndian(speaker, packet.data());
        for (const auto& other : std::as_const(peers_))
            if (other != peer && other->voice && other->voice->isOpen() &&
                other->voice->bufferedAmount() < MaxVoiceBuffer) other->voice->sendBuffer(packet);
    }
    if (speaker && speaker != selfId_) decodeVoice(speaker, packet.mid(4));
}

void VoiceParty::startAudio() {
    audio_ = std::make_unique<Audio>();
    if (qEnvironmentVariable("EMULOS_VOICE_NO_AUDIO") == QStringLiteral("1")) return;
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    const auto input = QMediaDevices::defaultAudioInput();
    const auto output = QMediaDevices::defaultAudioOutput();
    int error = OPUS_OK;
    audio_->encoder.reset(opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error));
    if (error != OPUS_OK || !audio_->encoder) {
        setStatus(tr("No se pudo iniciar el códec Opus.")); return;
    }
    opus_encoder_ctl(audio_->encoder.get(), OPUS_SET_BITRATE(32000));
    if (!input.isNull()) {
        audio_->input = std::make_unique<QAudioSource>(input, format);
        audio_->capture = audio_->input->start();
        if (audio_->capture) connect(audio_->capture, &QIODevice::readyRead, this, &VoiceParty::captureAudio);
    }
    if (!output.isNull()) {
        audio_->output = std::make_unique<QAudioSink>(output, format);
        audio_->output->setBufferSize(FrameBytes * 6);
        audio_->playback = audio_->output->start();
        if (audio_->playback) playback_.start();
    }
}

void VoiceParty::stopAudio() {
    if (!audio_) return;
    if (audio_->capture) disconnect(audio_->capture, nullptr, this, nullptr);
    if (audio_->input) audio_->input->stop();
    if (audio_->output) audio_->output->stop();
    audio_.reset();
}

void VoiceParty::captureAudio() {
    if (!audio_ || !audio_->capture || !audio_->encoder) return;
    audio_->pending += audio_->capture->readAll();
    while (audio_->pending.size() >= FrameBytes) {
        std::array<opus_int16, FrameSamples> samples{};
        std::memcpy(samples.data(), audio_->pending.constData(), FrameBytes);
        audio_->pending.remove(0, FrameBytes);
        if (muted_) continue;
        std::array<unsigned char, 1275> compressed{};
        const int size = opus_encode(audio_->encoder.get(), samples.data(), FrameSamples,
                                     compressed.data(), static_cast<opus_int32>(compressed.size()));
        if (size > 0) emitVoice(QByteArray(reinterpret_cast<const char*>(compressed.data()), size));
    }
}

void VoiceParty::decodeVoice(quint32 speaker, const QByteArray& opusFrame) {
    if (!audio_ || !audio_->playback || opusFrame.size() > 1275) return;
    auto it = audio_->decoders.find(speaker);
    if (it == audio_->decoders.end()) {
        int error = OPUS_OK;
        Audio::Decoder decoder(opus_decoder_create(48000, 1, &error), &opus_decoder_destroy);
        if (error != OPUS_OK || !decoder) return;
        it = audio_->decoders.emplace(speaker, std::move(decoder)).first;
    }
    std::array<opus_int16, FrameSamples> samples{};
    const int count = opus_decode(it->second.get(),
        reinterpret_cast<const unsigned char*>(opusFrame.constData()),
        static_cast<opus_int32>(opusFrame.size()), samples.data(), FrameSamples, 0);
    if (count != FrameSamples) return;
    auto& queue = audio_->frames[speaker];
    if (queue.size() >= 4) queue.pop_front();
    queue.push_back(samples);
}

void VoiceParty::playAudio() {
    if (!audio_ || !audio_->playback) return;
    std::array<opus_int32, FrameSamples> mixed{};
    bool any = false;
    for (auto& [speaker, queue] : audio_->frames) {
        Q_UNUSED(speaker)
        if (queue.empty()) continue;
        const auto& frame = queue.front();
        for (int i = 0; i < FrameSamples; ++i) mixed[i] += frame[i];
        queue.pop_front();
        any = true;
    }
    if (!any) return;
    std::array<opus_int16, FrameSamples> output{};
    for (int i = 0; i < FrameSamples; ++i)
        output[i] = static_cast<opus_int16>(std::clamp<opus_int32>(mixed[i], -32768, 32767));
    audio_->playback->write(reinterpret_cast<const char*>(output.data()), FrameBytes);
}

void VoiceParty::announce() {
    if (!hosting_ || !active_) return;
    const auto data = QJsonDocument(QJsonObject{{"app", "Emulos360Voice"}, {"version", 1},
                                                 {"name", displayName_}, {"port", static_cast<int>(port_)},
                                                 {"players", participants_.size()}})
                          .toJson(QJsonDocument::Compact);
    discovery_.writeDatagram(data, QHostAddress::Broadcast, DiscoveryPort);
    for (const auto& interface : QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsUp) ||
            (interface.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto& entry : interface.addressEntries())
            if (!entry.broadcast().isNull()) discovery_.writeDatagram(data, entry.broadcast(), DiscoveryPort);
    }
}

void VoiceParty::readDiscovery() {
    while (discovery_.hasPendingDatagrams()) {
        QByteArray data(static_cast<qsizetype>(discovery_.pendingDatagramSize()), '\0');
        QHostAddress sender;
        discovery_.readDatagram(data.data(), data.size(), &sender);
        if (data.size() > 512 || hosting_) continue;
        const auto json = QJsonDocument::fromJson(data).object();
        const auto port = json.value("port").toInt();
        if (json.value("app") != "Emulos360Voice" || json.value("version") != 1 ||
            port < 1 || port > 65535) continue;
        const auto address = sender.toString();
        const auto key = address + ':' + QString::number(port);
        discovered_[key] = QVariantMap{{"address", address}, {"port", port},
                                      {"name", safeName(json.value("name").toString())},
                                      {"players", json.value("players").toInt()}};
        seen_[key] = QDateTime::currentMSecsSinceEpoch();
    }
    updateNearby();
}

void VoiceParty::updateNearby() {
    if (hosting_) return;
    const auto now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = seen_.begin(); it != seen_.end();) {
        if (now - it.value() > 6000) { discovered_.remove(it.key()); it = seen_.erase(it); }
        else ++it;
    }
    QVariantList current;
    for (const auto& value : std::as_const(discovered_)) current.append(value);
    if (current == nearby_) return;
    nearby_ = current;
    emit changed();
}
