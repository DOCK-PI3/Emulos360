#include "game_importer.h"
#include "library.h"
#include "runtime_paths.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>
#include <QtConcurrentRun>

namespace {
bool safeArchiveListing(const QByteArray& listing, QString& error) {
    const auto lines = QString::fromUtf8(listing).split(u'\n');
    int entries = 0;
    for (const auto& raw : lines) {
        if (!raw.startsWith(QStringLiteral("Path = "))) continue;
        const auto path = QDir::fromNativeSeparators(raw.mid(7).trimmed());
        if (path.isEmpty() || path.startsWith(u'/') || path.contains(u':') ||
            path.split(u'/').contains(QStringLiteral(".."))) {
            error = QObject::tr("El archivo contiene una ruta insegura: %1").arg(path);
            return false;
        }
        if (++entries > 100000) {
            error = QObject::tr("El archivo contiene demasiadas entradas.");
            return false;
        }
    }
    if (entries == 0) error = QObject::tr("El 7z está vacío o no se pudo leer.");
    return entries > 0;
}

struct Candidate {
    QString path;
    los::Package package;
    bool iso = false;
};

bool packageComplete(const QString& path, const los::Package& package) {
    if (package.format == QStringLiteral("XBLA")) return QFileInfo(path).size() > 0x511;
    if (package.format != QStringLiteral("GOD")) return false;
    const QDir fragments(path + QStringLiteral(".data"));
    if (!fragments.exists()) return false;
    for (quint32 i = 0; i < package.fragments; ++i) {
        const QFileInfo part(fragments.filePath(QStringLiteral("Data%1").arg(i, 4, 10, QLatin1Char('0'))));
        if (!part.isFile() || part.isSymLink() || part.size() <= 0) return false;
    }
    return fragments.entryInfoList({QStringLiteral("Data*")}, QDir::Files | QDir::NoSymLinks).size() == package.fragments;
}

bool readPackage(const QString& path, Candidate& candidate) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    const auto package = los::parsePackageHeader(file.read(0x511));
    if (!package || !packageComplete(path, *package)) return false;
    candidate = Candidate{path, *package, false};
    return true;
}

QStringList filesUnder(const QString& directory, QString& error) {
    QStringList result;
    QDirIterator it(directory, QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System |
                         QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (info.isSymLink() || info.isJunction()) {
            error = QObject::tr("El archivo contiene un enlace o punto de unión no admitido.");
            return {};
        }
        if (!info.isFile()) continue;
        result.append(info.absoluteFilePath());
        if (result.size() > 100000) {
            error = QObject::tr("El archivo contiene demasiados ficheros.");
            return {};
        }
    }
    return result;
}

bool movePackage(const Candidate& candidate, const QString& libraryPath, QString& installedPath,
                 QString& error) {
    const QDir library(libraryPath);
    const QString type = candidate.package.format == QStringLiteral("GOD")
        ? QStringLiteral("00007000") : QStringLiteral("000D0000");
    const QString relative = candidate.package.titleId + u'/' + type;
    if (!QDir().mkpath(library.filePath(relative))) {
        error = QObject::tr("No se pudo crear la carpeta de destino.");
        return false;
    }
    installedPath = library.filePath(relative + u'/' + QFileInfo(candidate.path).fileName());
    const auto sourceData = candidate.path + QStringLiteral(".data");
    const auto targetData = installedPath + QStringLiteral(".data");
    const bool god = candidate.package.format == QStringLiteral("GOD");
    if (QFileInfo::exists(installedPath) || (god && QFileInfo::exists(targetData))) {
        error = QObject::tr("Este paquete ya existe en la biblioteca; no se ha sobrescrito.");
        return false;
    }
    if (god && !QDir().rename(sourceData, targetData)) {
        error = QObject::tr("No se pudieron mover los datos GOD.");
        return false;
    }
    if (!QFile::rename(candidate.path, installedPath)) {
        if (god) QDir().rename(targetData, sourceData);
        error = QObject::tr("No se pudo mover el paquete a la biblioteca.");
        return false;
    }
    if (!packageComplete(installedPath, candidate.package)) {
        QFile::rename(installedPath, candidate.path);
        if (god) QDir().rename(targetData, sourceData);
        error = QObject::tr("El paquete instalado no superó la comprobación.");
        return false;
    }
    return true;
}
}

GameImporter::GameImporter(QObject* parent) : QObject(parent) {
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, [this] {
        const auto result = watcher_.result();
        busy_ = false;
        status_ = result.message;
        progress_ = result.ok ? 1.0 : 0.0;
        emit changed();
        if (result.ok) emit installed();
        emit completed(result.ok);
    });
}

GameImporter::~GameImporter() {
    cancelled_.store(true);
    watcher_.waitForFinished();
}

void GameImporter::report(const QString& message, double progress) {
    QMetaObject::invokeMethod(this, [this, message, progress] {
        status_ = message;
        progress_ = progress;
        emit changed();
    }, Qt::QueuedConnection);
}

void GameImporter::cancel() { cancelled_.store(true); }

void GameImporter::importFile(const QUrl& source, const QString& libraryPath) {
    if (busy_) return;
    if (!source.isLocalFile() || !QFileInfo(source.toLocalFile()).isFile()) {
        status_ = tr("Selecciona un archivo local válido.");
        emit changed();
        return;
    }
    if (libraryPath.isEmpty() || !QFileInfo(libraryPath).isDir() || !QFileInfo(libraryPath).isWritable()) {
        status_ = tr("Configura primero una carpeta de biblioteca con permiso de escritura.");
        emit changed();
        return;
    }
    cancelled_.store(false);
    busy_ = true;
    progress_ = 0;
    status_ = tr("Preparando importación…");
    emit changed();
    watcher_.setFuture(QtConcurrent::run([this, input = source.toLocalFile(), libraryPath] {
        return importWorker(input, libraryPath);
    }));
}

GameImporter::Result GameImporter::importWorker(const QString& source, const QString& libraryPath) {
    const QDir library(libraryPath);
    const auto stage = library.filePath(QStringLiteral(".emulos-import-") +
                                        QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto fail = [this, &stage](const QString& message) -> Result {
        // The selected source is never inside the staging directory and is never removed.
        QDir(stage).removeRecursively();
        return {false, message};
    };
    if (!QDir().mkpath(stage)) return {false, tr("No se pudo preparar espacio temporal.")};
    auto run = [this](const QString& program, const QStringList& args, QByteArray& output) -> bool {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(program, args);
        if (!process.waitForStarted(10000)) { output = process.errorString().toUtf8(); return false; }
        process.closeWriteChannel();
        while (!process.waitForFinished(250)) {
            output += process.readAll();
            if (output.size() > 16 * 1024 * 1024) {
                process.kill();
                process.waitForFinished(5000);
                output = "La salida de la herramienta supera el límite permitido.";
                return false;
            }
            if (cancelled_.load()) {
                process.kill();
                process.waitForFinished(5000);
                return false;
            }
        }
        output += process.readAll();
        if (output.size() > 16 * 1024 * 1024) return false;
        return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    };

    QString input = source;
    const QFileInfo sourceInfo(source);
    if (sourceInfo.suffix().compare(QStringLiteral("7z"), Qt::CaseInsensitive) == 0) {
        const auto sevenZip = emulos::findImportTool(QStringLiteral("7z.exe"));
        if (sevenZip.isEmpty()) return fail(tr("No se puede ejecutar 7-Zip. Ejecuta el instalador de dependencias de la carpeta de Emulos360 para instalarlo y reparar los permisos."));
        report(tr("Comprobando el archivo 7z…"), 0.1);
        QByteArray listing;
        if (!run(sevenZip, {QStringLiteral("l"), QStringLiteral("-slt"), QStringLiteral("-ba"), source}, listing))
            return fail(cancelled_.load() ? tr("Importación cancelada.") : tr("No se pudo leer el 7z: %1").arg(QString::fromUtf8(listing.right(500))));
        QString error;
        if (!safeArchiveListing(listing, error)) return fail(error);
        const auto extracted = QDir(stage).filePath(QStringLiteral("extracted"));
        if (!QDir().mkpath(extracted)) return fail(tr("No se pudo crear la carpeta de extracción."));
        report(tr("Extrayendo el juego…"), 0.25);
        QByteArray output;
        if (!run(sevenZip, {QStringLiteral("x"), QStringLiteral("-y"), QStringLiteral("-aoa"),
                            QStringLiteral("-o") + extracted, source}, output))
            return fail(cancelled_.load() ? tr("Importación cancelada.") : tr("Falló la extracción: %1").arg(QString::fromUtf8(output.right(500))));
        input = extracted;
    }
    if (cancelled_.load()) return fail(tr("Importación cancelada."));

    Candidate candidate;
    if (QFileInfo(input).isDir()) {
        QString error;
        const auto files = filesUnder(input, error);
        if (!error.isEmpty()) return fail(error);
        QList<Candidate> candidates;
        for (const auto& path : files) {
            if (QFileInfo(path).suffix().compare(QStringLiteral("iso"), Qt::CaseInsensitive) == 0)
                candidates.append(Candidate{path, {}, true});
            else {
                Candidate found;
                if (readPackage(path, found)) candidates.append(found);
            }
        }
        if (candidates.size() != 1)
            return fail(tr("Se esperaba un juego ISO o paquete GOD/XBLA; encontrados: %1.").arg(candidates.size()));
        candidate = candidates.first();
    } else if (QFileInfo(input).suffix().compare(QStringLiteral("iso"), Qt::CaseInsensitive) == 0) {
        candidate = Candidate{input, {}, true};
    } else if (!readPackage(input, candidate)) {
        return fail(tr("Formato no reconocido. Selecciona una ISO, un 7z o un paquete GOD/XBLA válido."));
    } else {
        // A directly selected package belongs to the user. Install a staging copy.
        const auto staged = QDir(stage).filePath(QFileInfo(input).fileName());
        if (!QFile::copy(input, staged)) return fail(tr("No se pudo copiar el paquete al área temporal."));
        if (candidate.package.format == QStringLiteral("GOD")) {
            const auto originalData = input + QStringLiteral(".data");
            const auto stagedData = staged + QStringLiteral(".data");
            if (!QDir().mkpath(stagedData)) return fail(tr("No se pudieron copiar los datos GOD."));
            QString listingError;
            for (const auto& part : filesUnder(originalData, listingError)) {
                if (!QFile::copy(part, QDir(stagedData).filePath(QFileInfo(part).fileName())))
                    return fail(tr("No se pudieron copiar los datos GOD."));
            }
            if (!listingError.isEmpty()) return fail(listingError);
        }
        candidate.path = staged;
    }

    if (candidate.iso) {
        const auto converter = emulos::findImportTool(QStringLiteral("iso2god.exe"));
        if (converter.isEmpty()) return fail(tr("No se puede ejecutar ISO2GOD. Ejecuta el instalador de dependencias de la carpeta de Emulos360 para reparar los permisos, o reinstala la build completa."));
        const QStorageInfo disk(libraryPath);
        const qint64 minimum = QFileInfo(candidate.path).size() + 256LL * 1024 * 1024;
        if (disk.isValid() && disk.bytesAvailable() < minimum)
            return fail(tr("No hay espacio libre suficiente para convertir la ISO."));
        report(tr("Convirtiendo ISO a GOD en segundo plano…"), 0.55);
        const auto converted = QDir(stage).filePath(QStringLiteral("converted"));
        if (!QDir().mkpath(converted)) return fail(tr("No se pudo preparar la conversión."));
        QByteArray output;
        if (!run(converter, {candidate.path, converted}, output))
            return fail(cancelled_.load() ? tr("Importación cancelada.") :
                        tr("ISO2GOD no pudo convertir el juego: %1").arg(QString::fromUtf8(output.right(700))));
        const auto games = los::scanLibrary(converted).games;
        if (games.size() != 1 || !games.first().toMap().value(QStringLiteral("complete")).toBool() ||
            games.first().toMap().value(QStringLiteral("format")).toString() != QStringLiteral("GOD") ||
            !readPackage(games.first().toMap().value(QStringLiteral("path")).toString(), candidate))
            return fail(tr("La conversión terminó, pero el paquete GOD no pasó la comprobación."));
    }
    if (cancelled_.load()) return fail(tr("Importación cancelada."));
    report(tr("Instalando en la biblioteca…"), 0.9);
    QString installedPath, error;
    if (!movePackage(candidate, libraryPath, installedPath, error)) return fail(error);
    QDir(stage).removeRecursively();
    return {true, tr("Juego instalado en la biblioteca: %1").arg(candidate.package.title)};
}
