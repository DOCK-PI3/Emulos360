#pragma once
#include <QByteArray>
#include <QString>
#include <QVariantList>
#include <optional>

namespace los {
struct Package {
    QString titleId;
    QString title;
    quint32 fragments = 0;
    QString format;
};
struct ScanResult {
    QVariantList games;
    QString error;
};
std::optional<Package> parsePackageHeader(const QByteArray& header);
std::optional<Package> parseXexHeader(const QByteArray& header);
ScanResult scanLibrary(const QString& root);
}
