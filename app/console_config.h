#pragma once
#include <QByteArray>
#include <QVariantList>
#include <QVariantMap>

namespace los {
QByteArray consoleDefaults();
qsizetype consoleConfigSize();
QVariantList consoleCatalog(const QVariantList& countries);
QVariantMap readConsole(const QByteArray& bytes);
QByteArray writeConsole(const QByteArray& original, const QVariantMap& changed);
}
