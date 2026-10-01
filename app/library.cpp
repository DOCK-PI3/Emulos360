#include "library.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QtEndian>
#include <algorithm>
#include <utility>

namespace los {
namespace {
quint32 be32(const QByteArray& bytes, qsizetype offset) {
    return qFromBigEndian<quint32>(bytes.constData() + offset);
}
QString displayName(const QByteArray& header) {
    QString title;
    for (qsizetype offset = 0x411; offset < 0x511; offset += 2) {
        const auto code = qFromBigEndian<quint16>(header.constData() + offset);
        if (!code) break;
        title.append(QChar(code));
    }
    return title.trimmed();
}
}

std::optional<Package> parsePackageHeader(const QByteArray& header) {
    // XContentHeader/Metadata layout, referenced in engine/xenia/.../xcontent.h.
    // Discovery only: no signature, hash-tree or full-content verification.
    if (header.size() < 0x511) return std::nullopt;
    const auto magic = header.first(4);
    if (magic != "LIVE" && magic != "PIRS" && magic != "CON ") return std::nullopt;
    const auto contentType = be32(header, 0x344);
    const auto volumeType = be32(header, 0x3A9);
    const auto titleId = QString::number(be32(header, 0x360), 16).rightJustified(8, u'0').toUpper();
    const auto count = be32(header, 0x39D);
    if (contentType == 0x7000 && volumeType == 1 && count > 0 && count <= 10000)
        return Package{titleId, displayName(header), count, QStringLiteral("GOD")};
    if (contentType == 0xD0000 && volumeType == 0)
        return Package{titleId, displayName(header), 0, QStringLiteral("XBLA")};
    return std::nullopt;
}

std::optional<Package> parseXexHeader(const QByteArray& header) {
    // XEX2 main header and optional execution-info record. The module flags
    // distinguish launchable titles from DLLs/plugins shipped beside games.
    if (header.size() < 0x18 || header.first(4) != "XEX2") return std::nullopt;
    const auto flags = be32(header, 4);
    if (!(flags & 1) || (flags & (8 | 16 | 32 | 64))) return std::nullopt;
    const auto headerSize = be32(header, 8), count = be32(header, 0x14);
    if (headerSize < 0x18 || headerSize > 1024 * 1024 ||
        headerSize > static_cast<quint32>(header.size()) || count > 512 ||
        0x18u + count * 8u > headerSize) return std::nullopt;
    for (quint32 i = 0; i < count; ++i) {
        const qsizetype record = 0x18 + static_cast<qsizetype>(i) * 8;
        if (be32(header, record) != 0x00040006) continue;
        const auto offset = be32(header, record + 4);
        if (offset > headerSize || headerSize - offset < 0x18) return std::nullopt;
        const auto id = be32(header, offset + 0xC);
        // Exclude the system dashboard and the helper title found in FATX
        // backups, without rejecting valid game IDs by a broad bit pattern.
        if (!id || id == 0xFFFE07D1 || id == 0xF5D10000) return std::nullopt;
        return Package{QString::number(id, 16).rightJustified(8, u'0').toUpper(), {}, 0,
                       QStringLiteral("XEX")};
    }
    return std::nullopt;
}

ScanResult scanLibrary(const QString& root) {
    ScanResult result;
    const QDir rootDir(root);
    if (!rootDir.exists() || !QFileInfo(root).isReadable()) {
        result.error = QStringLiteral("No se puede acceder a la biblioteca: %1").arg(root);
        return result;
    }
    QStringList pending{rootDir.absolutePath()};
    QSet<QString> seen;
    QHash<QString, QVariantMap> xexTitles;
    int visited = 0;
    while (!pending.isEmpty()) {
        if (++visited > 100000) {
            result.error = QStringLiteral("La biblioteca supera el límite de exploración de este prototipo.");
            break;
        }
        const auto directory = pending.takeLast();
        // Xbox/FATX copy tools may preserve unusual Windows attributes (for
        // example pinned/offline) on otherwise regular 000D0000 directories.
        // Include those entries, then reject links explicitly.
        const auto entries = QDir(directory).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            if (entry.isSymLink()) continue;
            if (entry.isDir()) {
                if (!entry.fileName().endsWith(QStringLiteral(".data"), Qt::CaseInsensitive) &&
                    !entry.fileName().startsWith(QStringLiteral(".emulos-import-"), Qt::CaseInsensitive))
                    pending.append(entry.absoluteFilePath());
                continue;
            }
            if (entry.suffix().compare(QStringLiteral("xex"), Qt::CaseInsensitive) == 0) {
                QFile file(entry.absoluteFilePath());
                if (!file.open(QIODevice::ReadOnly)) continue;
                const auto package = parseXexHeader(file.read(1024 * 1024));
                if (!package) continue;
                const auto key = directory.toCaseFolded() + QLatin1Char('|') + package->titleId;
                const bool preferred = entry.fileName().compare(QStringLiteral("default.xex"), Qt::CaseInsensitive) == 0;
                if (xexTitles.contains(key) && !preferred) continue;
                QString title = QFileInfo(directory).fileName();
                if (title.isEmpty() || QFileInfo(directory).absoluteFilePath() == rootDir.absolutePath())
                    title = entry.completeBaseName();
                xexTitles.insert(key, QVariantMap{
                    {QStringLiteral("title"), title},
                    {QStringLiteral("titleId"), package->titleId},
                    {QStringLiteral("path"), entry.absoluteFilePath()},
                    {QStringLiteral("sizeGiB"), static_cast<double>(entry.size()) / (1024.0 * 1024.0 * 1024.0)},
                    {QStringLiteral("complete"), true},
                    {QStringLiteral("format"), QStringLiteral("XEX")},
                    {QStringLiteral("fragments"), 0}});
                continue;
            }
            const auto containerType = QFileInfo(directory).fileName().toUpper();
            if (containerType != QStringLiteral("00007000") && containerType != QStringLiteral("000D0000")) continue;
            QFile file(entry.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly)) continue;
            const auto package = parsePackageHeader(file.read(0x511));
            if (!package || seen.contains(entry.canonicalFilePath())) continue;
            if ((package->format == QStringLiteral("GOD") && containerType != QStringLiteral("00007000")) ||
                (package->format == QStringLiteral("XBLA") && containerType != QStringLiteral("000D0000"))) continue;
            seen.insert(entry.canonicalFilePath());
            bool complete = package->format == QStringLiteral("XBLA");
            quint64 bytes = static_cast<quint64>(entry.size());
            if (package->format == QStringLiteral("GOD")) {
                const QDir fragments(entry.absoluteFilePath() + QStringLiteral(".data"));
                const auto parts = fragments.entryInfoList({QStringLiteral("Data*")}, QDir::Files | QDir::NoSymLinks);
                complete = fragments.exists() && parts.size() == package->fragments;
                for (quint32 index = 0; index < package->fragments; ++index) {
                    const QFileInfo part(fragments.filePath(QStringLiteral("Data%1").arg(index, 4, 10, QLatin1Char('0'))));
                    if (!part.isFile() || part.isSymLink() || part.size() <= 0) complete = false;
                    else bytes += static_cast<quint64>(part.size());
                }
            }
            result.games.append(QVariantMap{
                {QStringLiteral("title"), package->title.isEmpty() ? package->titleId : package->title},
                {QStringLiteral("titleId"), package->titleId},
                {QStringLiteral("path"), entry.absoluteFilePath()},
                {QStringLiteral("sizeGiB"), static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0)},
                {QStringLiteral("complete"), complete},
                {QStringLiteral("format"), package->format},
                {QStringLiteral("fragments"), package->fragments}});
        }
    }
    for (const auto& game : std::as_const(xexTitles)) result.games.append(game);
    std::sort(result.games.begin(), result.games.end(), [](const QVariant& a, const QVariant& b) {
        return QString::localeAwareCompare(a.toMap().value(QStringLiteral("title")).toString(),
                                           b.toMap().value(QStringLiteral("title")).toString()) < 0;
    });
    return result;
}
}
