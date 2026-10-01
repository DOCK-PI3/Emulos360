// Emulos360 extension. XEX header layout: Xenia kernel/util/xex2_info.h.
#include "community_package.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>
#include <QtEndian>

namespace emulos {
namespace {
QVariantMap error(const QString& message) { return {{"complete",false},{"runtimeSupported",false},{"error",message}}; }
bool plainPath(const QString& path) {
    QFileInfo f(QFileInfo(path).absoluteFilePath());
    for (;;) {
        if (f.isSymLink()
#ifdef Q_OS_WIN
            || f.isJunction()
#endif
        ) return false;
        const auto parent=f.absolutePath();
        if (parent==f.absoluteFilePath()) return true;
        f=QFileInfo(parent);
    }
}
bool inventory(const QString& root,const QString& relative,QVariantList& files,qint64& total) {
    const QDir dir(QDir(root).filePath(relative));
    if (!dir.isReadable()) return false;
    for (const auto& item:dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,QDir::Name)) {
        if (!plainPath(item.absoluteFilePath())) return false;
        const auto name=relative+item.fileName();
        if (name=="emulos-package.json") continue;
        if (name.count('/')>16 || files.size()>=1000) return false;
        if (item.isDir()) {
            files.append(QVariantMap{{"path",name},{"directory",true}});
            if (!inventory(root,name+'/',files,total)) return false;
        } else {
            if (!item.isFile() || item.size()>32*1024*1024 || (total+=item.size())>128*1024*1024) return false;
            QFile f(item.absoluteFilePath()); if (!f.open(QIODevice::ReadOnly)) return false;
            const auto bytes=f.readAll(); if (bytes.size()!=item.size()) return false;
            QVariantMap row{{"path",name},{"bytes",item.size()},{"sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())}};
            if (item.suffix().compare("xex",Qt::CaseInsensitive)==0) row["xex"]=inspectXex(bytes);
            files.append(row);
        }
    }
    return true;
}
}
QVariantMap inspectXex(const QByteArray& bytes) {
    const auto size=static_cast<quint64>(bytes.size());
    const auto u32=[&](quint64 offset) { return qFromBigEndian<quint32>(bytes.constData()+offset); };
    if (size<24 || bytes.first(4)!="XEX2") return {{"valid",false},{"error","Cabecera XEX2 ausente o truncada"}};
    const auto headerSize=u32(8), count=u32(20), flags=u32(4);
    if (headerSize>size || headerSize<24 || count>(headerSize-24)/8) return {{"valid",false},{"error","Tabla XEX2 fuera de límites"}};
    QVariantMap result{{"valid",true},{"moduleFlags",QString::number(flags,16)},{"dll",bool(flags&8)}};
    for (quint32 i=0;i<count;++i) {
        const auto key=u32(24+i*8), offset=u32(28+i*8);
        if (key!=0x103ff && key!=0x3ff) continue;
        if (static_cast<quint64>(offset)+4>headerSize) return {{"valid",false},{"error","Cabecera opcional fuera de límites"}};
        const auto length=u32(offset);
        if (static_cast<quint64>(offset)+length>headerSize) return {{"valid",false},{"error","Cabecera opcional truncada"}};
        if (key==0x3ff) {
            if (length<8) return {{"valid",false}};
            result["encryption"]=qFromBigEndian<quint16>(bytes.constData()+offset+4);
            result["compression"]=qFromBigEndian<quint16>(bytes.constData()+offset+6);
        } else {
            if (length<12) return {{"valid",false}};
            const auto strings=u32(offset+4);
            if (strings>length-12) return {{"valid",false}};
            QStringList libraries;
            for (const auto& name:bytes.mid(offset+12,strings).split('\0')) if (!name.isEmpty()) libraries.append(QString::fromLatin1(name));
            result["libraries"]=libraries;
        }
    }
    return result;
}
QVariantMap inspectXbGuard(const QString& source,const QString& variant) {
    if (variant!="HDD" && variant!="USB") return error(QStringLiteral("Selecciona HDD o USB."));
    auto root=QFileInfo(source).absoluteFilePath();
    if (QFileInfo(QDir(root).filePath(variant)).isDir()) root=QDir(root).filePath(variant);
    if (!plainPath(root) || !QFileInfo(root).isDir()) return error(QStringLiteral("La carpeta no existe o contiene enlaces."));
    QVariantList files; qint64 total=0;
    if (!inventory(root,{},files,total)) return error(QStringLiteral("Paquete ilegible, con enlaces o demasiado grande (1000 entradas / 128 MiB)."));
    QMap<QString,QVariantMap> byName;
    for (const auto& row:files) {
        const auto item=row.toMap(); const auto name=item["path"].toString().toLower();
        if (byName.contains(name)) return error(QStringLiteral("Nombres duplicados al ignorar mayúsculas."));
        byName[name]=item;
    }
    if (!byName.contains("launch.ini") || byName["launch.ini"]["directory"].toBool()) return error(QStringLiteral("Falta launch.ini."));
    QFile ini(QDir(root).filePath(byName["launch.ini"]["path"].toString()));
    if (!ini.open(QIODevice::ReadOnly) || ini.size()>1024*1024) return error(QStringLiteral("No se puede leer launch.ini."));
    QMap<int,QString> pluginSlots; QString section;
    const auto text=QString::fromUtf8(ini.readAll());
    for (auto line:text.split('\n')) {
        line=line.trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;
        if (line.startsWith('[') && line.endsWith(']')) { section=line.mid(1,line.size()-2).trimmed().toLower(); continue; }
        if (section!="plugins") continue;
        const auto equals=line.indexOf('='); if (equals<0) continue;
        const auto key=line.first(equals).trimmed().toLower();
        if (!QRegularExpression("^plugin[1-5]$").match(key).hasMatch()) continue;
        const int slot=key.last(1).toInt();
        if (pluginSlots.contains(slot)) return error(QStringLiteral("Slot duplicado en launch.ini."));
        pluginSlots[slot]=line.mid(equals+1).section(';',0,0).trimmed();
    }
    QVariantList plugins; QStringList names;
    for (auto it=pluginSlots.cbegin();it!=pluginSlots.cend();++it) {
        if (it.value().isEmpty()) continue;
        const auto match=QRegularExpression("^(Hdd|Usb):[\\\\/](.+)$",QRegularExpression::CaseInsensitiveOption).match(it.value());
        if (!match.hasMatch() || match.captured(1).compare(variant,Qt::CaseInsensitive)!=0) return error(QStringLiteral("Las rutas de launch.ini no corresponden a la variante seleccionada."));
        auto path=match.captured(2); path.replace('\\','/');
        if (path.contains(':') || path.startsWith('/') || path.split('/').contains("..") || path.split('/').contains(".")) return error(QStringLiteral("Ruta de plugin fuera del paquete."));
        const auto item=byName.value(path.toLower()); const auto xex=item.value("xex").toMap();
        if (item.isEmpty() || !xex.value("valid").toBool()) return error(QStringLiteral("Falta un plugin o su cabecera es inválida: ")+path);
        plugins.append(QVariantMap{{"slot",it.key()},{"guestPath",it.value()},{"file",item["path"]},{"dll",xex["dll"]},{"libraries",xex["libraries"]}});
        names.append(QFileInfo(path).fileName().toLower());
    }
    for (const auto& required:{"xbdm.xex","xbguard.xex","jrpc2.xex"}) if (!names.contains(QString::fromLatin1(required))) return error(QStringLiteral("El paquete debe declarar xbdm, xbGuard y JRPC2 en launch.ini."));
    return {{"complete",true},{"runtimeSupported",false},{"variant",variant},{"source",root},{"files",files},{"plugins",plugins},
        {"status",QStringLiteral("Paquete completo · Ejecución no compatible todavía")},
        {"reason",QStringLiteral("Esta base de Xenia no implementa el arranque de plugins de DashLaunch. Importar los archivos no habilita la autenticación de XBGuard.")}};
}
QVariantMap importXbGuard(const QString& source,const QString& variant,const QString& destination) {
    const auto inspected=inspectXbGuard(source,variant);
    if (!inspected["complete"].toBool()) return inspected;
    const auto root=QDir(destination).filePath(variant.toLower()+'-'+QUuid::createUuid().toString(QUuid::Id128));
    if (!plainPath(root) || !QDir().mkpath(root)) return error(QStringLiteral("No se puede crear la carpeta de importación."));
    for (const auto& row:inspected["files"].toList()) {
        const auto item=row.toMap(); const auto target=QDir(root).filePath(item["path"].toString());
        if (item["directory"].toBool()) { if (!QDir().mkpath(target)) return error(QStringLiteral("No se pudo conservar una carpeta.")); continue; }
        const auto original=QDir(inspected["source"].toString()).filePath(item["path"].toString());
        if (!plainPath(original) || !QFile::copy(original,target)) return error(QStringLiteral("No se pudo copiar el paquete completo."));
    }
    auto copied=inspectXbGuard(root,variant);
    if (!copied["complete"].toBool() || copied["files"]!=inspected["files"] || copied["plugins"]!=inspected["plugins"]) return error(QStringLiteral("El paquete cambió durante la importación. No se activa."));
    copied["originalSource"]=inspected["source"]; copied["installedPath"]=root;
    const auto manifest=QJsonDocument::fromVariant(copied).toJson();
    QSaveFile file(root+"/emulos-package.json");
    if (!file.open(QIODevice::WriteOnly) || file.write(manifest)!=manifest.size() || !file.commit()) return error(QStringLiteral("No se pudo guardar el manifiesto del paquete."));
    return copied;
}
}
