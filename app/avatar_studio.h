#pragma once
#include <QObject>
#include <QColor>
#include <QVariantList>
#include <QQuick3DGeometry>
#include <QQuick3DTextureData>
#include <QtQml/qqmlregistration.h>
#include <QTimer>
#include <QElapsedTimer>
#include "xenia/kernel/xam/emulos_avatar_scene.h"
#include "xenia/kernel/xam/emulos_avatar_animation.h"

class AvatarStudio final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(AvatarStudio)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(QVariantList parts READ parts NOTIFY sceneChanged)
    Q_PROPERTY(QVariantList choices READ choices NOTIFY choicesChanged)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString profile READ profile NOTIFY changed)
    Q_PROPERTY(int body READ body NOTIFY changed)
    Q_PROPERTY(int category READ category WRITE setCategory NOTIFY choicesChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(QVariantList colors READ colors NOTIFY changed)
    Q_PROPERTY(bool animated READ animated NOTIFY changed)
    Q_PROPERTY(QVariantList animations READ animations NOTIFY changed)
    Q_PROPERTY(QString animationId READ animationId NOTIFY changed)
public:
    explicit AvatarStudio(const QString& dataRoot, QObject* parent = nullptr);
    QVariantList parts() const { return parts_; }
    QVariantList choices() const { return choices_; }
    QVariantList colors() const;
    QString status() const { return status_; }
    QString profile() const { return profile_; }
    int body() const;
    int category() const { return category_; }
    bool ready() const { return ready_; }
    bool dirty() const { return dirty_; }
    bool animated() const { return animation_.has_value() && skeleton_.has_value(); }
    QVariantList animations() const;
    QString animationId() const { return animationId_; }
    void openProfile(const QString& xuid, const QByteArray& manifest);
    void saved();
    QString resourceRoot() const;
    Q_INVOKABLE void setCategory(int category);
    Q_INVOKABLE void setBody(int body);
    Q_INVOKABLE void choose(const QString& id);
    Q_INVOKABLE void setColor(int index, const QColor& color);
    Q_INVOKABLE void save();
    Q_INVOKABLE void setPreviewActive(bool active) { previewActive_=active; if(active&&animated())animationTimer_.start();else animationTimer_.stop(); }
    Q_INVOKABLE void setAnimation(const QString& id);
    Q_INVOKABLE void preview() { openProfile({},{}); }
signals:
    void opened();
    void changed();
    void sceneChanged();
    void choicesChanged();
    void saveRequested(const QString& xuid, const QByteArray& manifest);
private:
    bool loadCatalog();
    bool rebuild();
    void refreshChoices();
    void animate();
    QString dataRoot_, profile_, status_, animationId_;
    int category_ = 0;
    bool ready_ = false, dirty_ = false, previewActive_ = false;
    std::optional<xe::kernel::xam::emulos::AvatarAssetCatalog> catalog_;
    xe::kernel::xam::emulos::AvatarManifest manifest_{};
    QVariantList parts_, choices_;
    QList<QQuick3DObject*> objects_;
    struct MeshFrame {QQuick3DGeometry* geometry;QByteArray original;std::vector<xe::kernel::xam::emulos::AvatarVertex> vertices;};
    std::vector<MeshFrame> meshes_;
    std::optional<xe::kernel::xam::emulos::AvatarSkeleton> skeleton_;
    std::optional<xe::kernel::xam::emulos::AvatarAnimation> animation_;
    QTimer animationTimer_;
    QElapsedTimer animationClock_;
};
