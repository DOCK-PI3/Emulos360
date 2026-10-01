#pragma once
#include <QByteArray>
#include <QString>
#include <QVariantMap>

namespace emulos {
// Static inspection only; no guest code runs and no credentials are read.
QVariantMap inspectXex(const QByteArray& bytes);
QVariantMap inspectXbGuard(const QString& source, const QString& variant);
QVariantMap importXbGuard(const QString& source, const QString& variant, const QString& destination);
}
