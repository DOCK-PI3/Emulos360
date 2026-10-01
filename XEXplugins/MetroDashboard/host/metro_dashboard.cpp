#include "metro_dashboard.h"
#include "guide_hold.h"
#include "controller.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QSaveFile>
#include <QUuid>
#include <QUrl>
#include <QElapsedTimer>
#include <chrono>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#endif

namespace {
bool write(const QString& path,const QByteArray& bytes) {
    QSaveFile f(path); return f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size() && f.commit();
}
bool writeShared(const QString& path,const QByteArray& bytes) {
    // Xenia opens these files every frame without FILE_SHARE_DELETE. Truncate
    // in place; a partial read is ignored and retried on the next guest tick.
    QFile f(path);
    return f.open(QIODevice::WriteOnly|QIODevice::Truncate) &&
           f.write(bytes)==bytes.size() && f.flush();
}
QByteArray line(QString value,int limit) {
    value.replace('\n',' '); value.replace('\r',' '); value.replace(QChar(0),' ');
    return value.left(limit).toLatin1()+'\n';
}
}
MetroDashboard::MetroDashboard(Controller* controller,const QString& data,QObject* parent)
    : QObject(parent),controller_(controller),root_(QDir(data).absoluteFilePath("metro-session")) {
    timer_.setInterval(150);
    connect(&timer_,&QTimer::timeout,this,&MetroDashboard::poll);
    connect(controller_->players(),&PlayerServices::sessionEnded,this,&MetroDashboard::ended);
    connect(controller_->netplayRooms(),&NetplayRooms::changed,this,[this] { if(active_) snapshot(); });
    closeTimeout_.setSingleShot(true); closeTimeout_.setInterval(15000);
    connect(&closeTimeout_,&QTimer::timeout,this,[this] {
        pending_=0; message(tr("El motor sigue abierto. Cierralo para continuar; no se ha forzado su cierre."));
    });
#ifdef Q_OS_WIN
    // Same Windows Guide-button extension already used by Xenia's XInput driver.
    using StateFn=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
    HMODULE module=LoadLibraryW(L"xinput1_3.dll");
    StateFn state=module?reinterpret_cast<StateFn>(GetProcAddress(module,MAKEINTRESOURCEA(100))):nullptr;
    if(state) {
        auto guide=new QTimer(this); guide->setInterval(30);
        connect(guide,&QTimer::timeout,this,[this,state,hold=MetroGuideHold{}]() mutable {
            DWORD owner=0; GetWindowThreadProcessId(GetForegroundWindow(),&owner);
            int down=-1;
            for(DWORD i=0;i<4;++i) { XINPUT_STATE s{}; if(state(i,&s)==ERROR_SUCCESS && (s.Gamepad.wButtons&0x0400)) { down=int(i); break; } }
            const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            const bool focused=owner==GetCurrentProcessId() || owner==controller_->players()->sessionProcessId();
            if(hold.update(down,active_ && focused,now)) openPower();
        });
        guide->start();
    }
    if(module) connect(this,&QObject::destroyed,[module] { FreeLibrary(module); });
#endif
}
void MetroDashboard::message(const QString& text) { status_=text; emit changed(); }
bool MetroDashboard::decodeRequest(const QByteArray& bytes,const QString& nonce,quint32 previous,
                                  int gameCount,quint32& serial,int& action,int& argument) {
    if(bytes.size()>256) return false;
    const auto fields=bytes.split('\n');
    if(fields.size()!=7 || fields[0]!="EMETRO1" || fields[1]!=nonce.toLatin1() ||
       fields[5]!="END" || !fields[6].isEmpty() || nonce.size()!=32) return false;
    bool s=false,a=false,i=false;
    const quint32 seq=fields[2].toUInt(&s); const int cmd=fields[3].toInt(&a),index=fields[4].toInt(&i);
    // The guest can request the power menu, never shutdown Windows directly.
    if(!s || !a || !i || seq<=previous || cmd<1 || cmd>11 ||
       ((cmd==1 || cmd==11) ? index<0 || index>=gameCount : index!=0)) return false;
    serial=seq; action=cmd; argument=index; return true;
}
bool MetroDashboard::snapshot() {
    QByteArray bytes="EMETRO1\n"+nonce_.toLatin1()+'\n';
    QString profile=tr("Sin perfil");
    for(const auto& p:controller_->players()->profiles()) {
        const auto row=p.toMap(); if(row["xuid"].toString()==controller_->players()->activeXuid()) profile=row["gamertag"].toString();
    }
    bytes+=line(profile,64)+line(controller_->netplayRooms()->status(),160);
    bytes+=QByteArray::number(games_.size())+'\n';
    for(const auto& g:games_) { const auto row=g.toMap(); bytes+=line(row["title"].toString(),100)+line(row["titleId"].toString(),8); }
    const auto rooms=controller_->netplayRooms()->rooms();
    const auto count=qMin<qsizetype>(rooms.size(),256);
    bytes+=QByteArray::number(count)+'\n';
    for(qsizetype i=0;i<count;++i) {
        const auto room=rooms[i].toMap(); int index=-1;
        for(qsizetype j=0;j<games_.size();++j) if(games_[j].toMap()["titleId"].toString().compare(room["titleId"].toString(),Qt::CaseInsensitive)==0) { index=int(j); break; }
        bytes+=line(room["title"].toString()+" - "+room["host"].toString(),100)+QByteArray::number(index+1)+'\n';
    }
    return mode_==1 ? writeShared(root_+"/snapshot.txt",bytes) : write(root_+"/snapshot.txt",bytes);
}
void MetroDashboard::start() {
    if(controller_->players()->busy() || controller_->busy()) { message(tr("Espera a que termine la sesion o la carga de la biblioteca.")); return; }
    games_=controller_->games().mid(0,1024);
    const QString source=QCoreApplication::applicationDirPath()+"/XEXplugins/MetroDashboard";
    if(!QDir().mkpath(root_+"/covers")) { message(tr("No se pudo preparar el dashboard.")); return; }
    for(const auto& name:{QString("default.xex"),QString("font.png")}) {
        QFile f(source+'/'+name);
        if(!f.open(QIODevice::ReadOnly) || !write(root_+'/'+name,f.readAll())) { message(tr("Falta el paquete Dashboard Metro: %1").arg(source)); return; }
    }
    active_=true; launchDashboard();
}
void MetroDashboard::syncCovers() {
    const auto images=controller_->coverImages();
    for(qsizetype i=0;i<games_.size();++i) {
        const QString target=root_+"/covers/"+QString::number(i)+".png";
        QFile::remove(target);
        const auto id=games_[i].toMap()["titleId"].toString().toUpper();
        const QImage image(QUrl(images.value(id).toString()).toLocalFile());
        if(!image.isNull()) image.scaled(256,360,Qt::IgnoreAspectRatio,Qt::SmoothTransformation).save(target);
    }
}
void MetroDashboard::launchDashboard() {
    if(!active_) return;
    syncCovers();
    nonce_=QUuid::createUuid().toString(QUuid::Id128); serial_=0; guideSerial_=0; powerArmed_=false;
    QFile::remove(root_+"/request.txt");
    // Xenia indexes host files when it mounts game:. Keep these entries
    // present before launch so later host writes remain visible to the guest.
    if(!writeShared(root_+"/ack.txt",{}) || !writeShared(root_+"/guide.txt",{})) {
        active_=false; message(tr("No se pudo preparar la comunicacion con el dashboard.")); return;
    }
    if(!snapshot()) { active_=false; message(tr("No se pudo guardar la biblioteca del dashboard.")); return; }
    mode_=1; controller_->players()->launchMetro(root_+"/default.xex");
    if(!controller_->players()->sessionActive()) { active_=false; mode_=0; message(controller_->players()->status()); return; }
    timer_.start(); message(tr("Dashboard Metro activo. Mantén Guia 2,5 segundos para abrir Sistema."));
}
void MetroDashboard::poll() {
    if(!active_ || mode_!=1 || pending_ || !controller_->players()->sessionActive()) return;
    QFile f(root_+"/request.txt");
    if(!f.open(QIODevice::ReadOnly)) return;
    const auto bytes=f.read(257); int action=0,index=0; quint32 sequence=0;
    if(!decodeRequest(bytes,nonce_,serial_,int(games_.size()),sequence,action,index)) return;
    if(action==6 && !powerArmed_) return;
    if(!writeShared(root_+"/ack.txt",QByteArray::number(sequence))) return;
    serial_=sequence;
    if(action==6) powerArmed_=false;
    if(action==2) controller_->netplayRooms()->refresh();
    else if(action==9) openPower();
    else transition(action,index);
}
void MetroDashboard::transition(int action,int argument) {
    if(pending_) return;
    pending_=action; argument_=argument;
    if(controller_->players()->sessionActive()) {
        controller_->players()->coreWindow()->requestClose(); closeTimeout_.start();
    } else dispatch();
}
void MetroDashboard::ended(int code) {
    closeTimeout_.stop(); timer_.stop();
    if(pending_) { dispatch(); return; }
    if(!active_) return;
    if(mode_==1) { active_=false; mode_=0; message(code?tr("El dashboard termino con un error. Revisa core.log."):tr("Dashboard cerrado.")); return; }
    mode_=0;
    QTimer::singleShot(0,this,&MetroDashboard::launchDashboard);
}
void MetroDashboard::dispatch() {
    const int action=pending_,index=argument_; pending_=0; mode_=0;
    if(action==11 && index>=0 && index<games_.size()) {
        mode_=4;
        emit editCover(games_[index].toMap());
    } else if(action==1 && index>=0 && index<games_.size()) {
        const auto g=games_[index].toMap(); mode_=2;
        controller_->players()->launchGame(g["path"].toString(),g["titleId"].toString());
        if(!controller_->players()->sessionActive()) { message(controller_->players()->status()); QTimer::singleShot(0,this,&MetroDashboard::launchDashboard); }
    } else if(action==3) {
        mode_=3; controller_->players()->launchNetplay();
        if(!controller_->players()->sessionActive()) QTimer::singleShot(0,this,&MetroDashboard::launchDashboard);
    } else if(action==5) QTimer::singleShot(0,this,&MetroDashboard::launchDashboard);
    else if(action==6) {
        active_=false;
#ifdef Q_OS_WIN
        const auto program=QDir(qEnvironmentVariable("SystemRoot")).filePath("System32/shutdown.exe");
        // No /f: Windows can protect applications with unsaved documents.
        if(!QProcess::startDetached(program,{"/s","/t","0"})) message(tr("Windows no pudo iniciar el apagado."));
#endif
    } else { active_=false; emit showPage("library"); }
    emit changed();
}
void MetroDashboard::finishCoverEdit() {
    if(!active_ || mode_!=4 || pending_) return;
    mode_=0;
    QTimer::singleShot(0,this,&MetroDashboard::launchDashboard);
}
void MetroDashboard::openPower() {
    if(!active_ || powerOpen_ || pending_) return;
    if(mode_==1) {
        powerArmed_=true;
        ++guideSerial_;
        if(!writeShared(root_+"/guide.txt",nonce_.toLatin1()+'\n'+QByteArray::number(guideSerial_)+'\n')) {
            powerArmed_=false; --guideSerial_;
            message(tr("No se pudo abrir la guia del dashboard."));
        }
        return;
    }
    powerOpen_=true; emit changed();
}
void MetroDashboard::cancelPower() {
    powerOpen_=false; emit changed();
    QTimer::singleShot(0,controller_->players()->coreWindow(),&CoreWindowHost::focus);
}
void MetroDashboard::confirmPower(int action) {
    if(!powerOpen_ || (action!=4 && action!=5 && action!=6)) return;
    powerOpen_=false; emit changed(); transition(action);
}
