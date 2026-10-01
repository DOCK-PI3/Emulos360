#pragma once
#include <QString>
#include <QVariantList>
namespace emulos {
QVariantList savedGames(const QString& content);
QVariantList saveBackups(const QString& root);
QString backupSave(const QString& content, const QString& root, const QString& xuid, const QString& title);
QString restoreSave(const QString& content, const QString& root, const QString& backupId);
}
