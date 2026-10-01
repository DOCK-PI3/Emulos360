#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

namespace emulos {

inline bool runnableFile(const QString& path) {
    const QFileInfo info(path);
#ifdef Q_OS_WIN
    return info.isFile();
#else
    return info.isFile() && info.isExecutable();
#endif
}

inline QString findImportTool(const QString& name,
                              const QString& applicationRoot = QCoreApplication::applicationDirPath()) {
    const QDir app(applicationRoot);
#ifdef Q_OS_WIN
    const QStringList candidates{name};
#else
    const QStringList candidates = name == QStringLiteral("7z.exe")
        ? QStringList{QStringLiteral("7zz"), QStringLiteral("7z"), QStringLiteral("7za")}
        : name == QStringLiteral("iso2god.exe")
            ? QStringList{QStringLiteral("iso2god")}
            : QStringList{name};
#endif
    for (const auto& candidate : candidates) {
        const auto bundled = app.filePath(QStringLiteral("tools/") + candidate);
        if (runnableFile(bundled)) return bundled;
    }
    if (name == QStringLiteral("iso2god.exe")) {
#ifdef Q_OS_WIN
        const auto developer = app.filePath(QStringLiteral("../../.tools/iso2god-rs/target/release/iso2god.exe"));
#else
        const auto developer = app.filePath(QStringLiteral("../../.tools/iso2god-rs/target/release/iso2god"));
#endif
        if (runnableFile(developer)) return developer;
    }
#ifdef Q_OS_WIN
    if (name == QStringLiteral("7z.exe")) {
        const auto installed = QStringLiteral("C:/Program Files/7-Zip/7z.exe");
        if (runnableFile(installed)) return installed;
    }
#endif
    for (const auto& candidate : candidates) {
        const auto executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty()) return executable;
    }
    return {};
}

inline QString avatarResourceRoot(const QString& dataRoot,
                                  const QString& applicationRoot = QCoreApplication::applicationDirPath()) {
    const auto personal = QDir(dataRoot).filePath(QStringLiteral("avatar-system"));
    // Keep an existing user catalog authoritative, including its validation errors.
    if (QFileInfo::exists(personal + QStringLiteral("/AvatarAssetPack.toc"))) return personal;
    const auto packaged = QDir(applicationRoot).filePath(QStringLiteral("assets/avatar-system"));
    if (QFileInfo::exists(packaged + QStringLiteral("/AvatarAssetPack.toc"))) return packaged;
    return personal;
}

} // namespace emulos
