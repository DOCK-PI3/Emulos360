#include "controller.h"
#include "community_package.h"
#include "gamepad_navigation.h"
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTimer>
#include <cstdio>

int main(int argc, char* argv[]) {
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
        const auto text = message.toUtf8();
        std::fprintf(stderr, "%s\n", text.constData());
    });
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    QCoreApplication::setApplicationName("Emulos360");
    QCoreApplication::setApplicationVersion(EMULOS_VERSION);
    QCoreApplication::setOrganizationName("Emulos360");
    app.setWindowIcon(QIcon(":/assets/emulos360.png"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("library", "Initial Xbox 360 GOD/XBLA/XEX library folder", "path"));
    parser.addOption(QCommandLineOption("data-dir", "Local settings directory", "path"));
    parser.addOption(QCommandLineOption("inventory", "Print JSON inventory and exit"));
    parser.addOption(QCommandLineOption("capture", "Save screenshot and exit", "path"));
    parser.addOption(QCommandLineOption("capture-intro", "Save a frame of the selected startup intro and exit", "path"));
    parser.addOption(QCommandLineOption("metro", "Open the native Metro XEX dashboard"));
    parser.addOption(QCommandLineOption("console", "Open in console mode"));
    parser.addOption(QCommandLineOption("avatar-preview", "Open original 3D avatar preview without modifying a profile"));
    parser.addOption(QCommandLineOption("avatar-manifest", "Preview an original avatar manifest without saving it", "path"));
    parser.addOption(QCommandLineOption("page", "Initial page: library, store, import, achievements, settings, profiles, saves, community", "page", "library"));
    parser.process(app);
    if (parser.isSet("inventory")) {
        const auto result = los::scanLibrary(parser.value("library"));
        const auto data = QJsonDocument(QJsonObject::fromVariantMap({{"games", result.games}, {"error", result.error}})).toJson();
        std::fwrite(data.constData(), 1, static_cast<size_t>(data.size()), stdout);
        return result.error.isEmpty() ? 0 : 1;
    }
    const auto dataDir = parser.isSet("data-dir") ? parser.value("data-dir") : app.applicationDirPath() + "/data";
    if (!QDir().mkpath(dataDir)) return 2;
    Controller controller(QDir(dataDir).filePath("settings.ini"));
    installGamepadNavigation(&controller,&app);
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}, {"initialPage",parser.value("page")},
                                 {"avatarPreview",parser.isSet("avatar-preview")},
                                 {"introEnabled",!parser.isSet("capture") && !parser.isSet("avatar-preview")}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(3); }, Qt::QueuedConnection);
    engine.loadFromModule("Los", "Main");
    if(parser.isSet("avatar-manifest")) {
        QFile manifest(parser.value("avatar-manifest"));
        if(!manifest.open(QIODevice::ReadOnly)||manifest.size()!=1000)return 7;
        controller.players()->avatarStudio()->openProfile({},manifest.readAll());
    }
    if (parser.isSet("library")) controller.setLibraryPath(parser.value("library"));
    else controller.scan();
    QTimer::singleShot(0, controller.players(), &PlayerServices::refreshProfiles);
    if (parser.isSet("console") || parser.isSet("metro")) {
        auto metroStart = new QTimer(&app);
        metroStart->setInterval(200);
        QObject::connect(metroStart,&QTimer::timeout,&app,[&controller,&engine,metroStart] {
            if(controller.busy() || controller.players()->busy()) return;
            if (!engine.rootObjects().isEmpty() && !engine.rootObjects().first()->property("introFinished").toBool()) return;
            metroStart->stop(); controller.metro()->start();
        });
        metroStart->start();
    }
    if (parser.isSet("capture")) {
        const auto path = parser.value("capture");
        auto captureTimer = new QTimer(&app);
        captureTimer->setInterval(500);
        QObject::connect(captureTimer, &QTimer::timeout, &app, [&engine, &controller, path, attempts=0]() mutable {
            if ((controller.busy() || controller.players()->busy() || controller.netplayRooms()->busy() || controller.store()->busy()) && ++attempts < 120) return;
            if (engine.rootObjects().isEmpty()) { QCoreApplication::exit(4); return; }
            const auto window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            QCoreApplication::exit(window && window->grabWindow().save(path) ? 0 : 5);
        });
        QTimer::singleShot(2500, captureTimer, [captureTimer] { captureTimer->start(); });
    }
    if (parser.isSet("capture-intro")) {
        const auto path = parser.value("capture-intro");
        QTimer::singleShot(3500, &app, [&engine, path] {
            if (engine.rootObjects().isEmpty()) { QCoreApplication::exit(4); return; }
            const auto window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            QCoreApplication::exit(window && window->grabWindow().save(path) ? 0 : 5);
        });
    }
    return app.exec();
}
