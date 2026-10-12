#include "gamepad_mapper.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <QTemporaryDir>
#include <QTest>
#include <QFile>
#include <QScopeGuard>

class GamepadMapperTests final : public QObject {
    Q_OBJECT
    QString source(const GamepadMapper& mapper, const QString& key) {
        for (const auto& control : mapper.controls()) {
            const auto row = control.toMap();
            if (row["key"] == key) return row["source"].toString();
        }
        return {};
    }
private slots:
    void initTestCase() {
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        QVERIFY(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0);
    }
    void cleanupTestCase() { SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); }
    void buttonsAxesHatPersistenceAndCoreConsumption() {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        QVERIFY(index >= 0);
        auto* joystick = SDL_JoystickOpen(index); QVERIFY(joystick);
        const auto cleanup = qScopeGuard([joystick, index] { SDL_JoystickClose(joystick); SDL_JoystickDetachVirtual(index); });
        SDL_JoystickSetVirtualAxis(joystick, 4, -32768);
        SDL_JoystickSetVirtualAxis(joystick, 5, -32768);
        SDL_JoystickUpdate();
        QTemporaryDir data; GamepadMapper mapper(data.path()); mapper.begin();
        QVERIFY(mapper.active()); QCOMPARE(mapper.controls().size(), 25);
        int selected = -1;
        for (int i = 0; i < mapper.devices().size(); ++i)
            if (mapper.devices()[i].toMap()["index"].toInt() == index) selected = i;
        QVERIFY(selected >= 0); mapper.selectDevice(selected);
        const auto guid = mapper.devices()[selected].toMap()["guid"].toString();
        QVERIFY(GamepadMapper::launchArguments(data.path()).isEmpty());
        mapper.capture("a"); mapper.poll();
        SDL_JoystickSetVirtualButton(joystick, 1, 1); mapper.poll();
        QCOMPARE(source(mapper, "a"), QString("b1")); QVERIFY(mapper.target().isEmpty());
        mapper.poll(); QCOMPARE(mapper.values()["a"].toDouble(), 1.0);
        SDL_JoystickSetVirtualButton(joystick, 1, 0); mapper.poll();
        QCOMPARE(mapper.values()["a"].toDouble(), 0.0);
        mapper.capture("dpright"); mapper.poll();
        SDL_JoystickSetVirtualHat(joystick, 0, SDL_HAT_RIGHT); mapper.poll();
        QCOMPARE(source(mapper, "dpright"), QString("h0.2"));
        SDL_JoystickSetVirtualHat(joystick, 0, SDL_HAT_CENTERED); mapper.poll();
        mapper.capture("-leftx"); mapper.poll();
        SDL_JoystickSetVirtualAxis(joystick, 0, -26000); mapper.poll();
        QCOMPARE(source(mapper, "-leftx"), QString("-a0"));
        mapper.poll(); QVERIFY(mapper.values()["-leftx"].toDouble() > 0.7);
        SDL_JoystickSetVirtualAxis(joystick, 0, 0); mapper.poll();
        mapper.capture("+leftx"); mapper.poll();
        SDL_JoystickSetVirtualAxis(joystick, 0, 26000); mapper.poll();
        QCOMPARE(source(mapper, "+leftx"), QString("+a0"));
        SDL_JoystickSetVirtualAxis(joystick, 0, 0); mapper.poll();
        mapper.capture("lefttrigger"); mapper.poll();
        SDL_JoystickSetVirtualAxis(joystick, 4, 32767); mapper.poll();
        QCOMPARE(source(mapper, "lefttrigger"), QString("a4"));
        mapper.poll(); QVERIFY(mapper.values()["lefttrigger"].toDouble() > 0.99);
        SDL_JoystickSetVirtualAxis(joystick, 4, -32768); mapper.poll();
        QVERIFY(mapper.dirty()); QVERIFY2(mapper.save(), qPrintable(mapper.status())); QVERIFY(!mapper.dirty());
        const auto arguments = GamepadMapper::launchArguments(data.path());
        QVERIFY(arguments.contains("--hid=any"));
        QFile file(data.filePath("controllers/gamecontrollerdb.txt")); QVERIFY(file.open(QIODevice::ReadOnly));
        const auto line = file.readAll().trimmed(); QVERIFY(line.startsWith(guid.toUtf8()));
        QVERIFY(SDL_GameControllerAddMapping(line.constData()) >= 0);
        auto* gamepad = SDL_GameControllerOpen(index); QVERIFY(gamepad);
        const auto closeGamepad = qScopeGuard([gamepad] { SDL_GameControllerClose(gamepad); });
        SDL_JoystickSetVirtualButton(joystick, 1, 1); SDL_GameControllerUpdate();
        QCOMPARE(SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A), Uint8(1));
        SDL_JoystickSetVirtualButton(joystick, 1, 0);
        SDL_JoystickSetVirtualAxis(joystick, 0, -26000); SDL_GameControllerUpdate();
        QVERIFY(SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX) < -20000);
        SDL_JoystickSetVirtualAxis(joystick, 0, 26000); SDL_GameControllerUpdate();
        QVERIFY(SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX) > 20000);
        SDL_JoystickSetVirtualAxis(joystick, 4, 32767); SDL_GameControllerUpdate();
        QVERIFY(SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 30000);
        mapper.end(); QVERIFY(!mapper.active());
        GamepadMapper reopened(data.path()); reopened.begin(); reopened.selectDevice(selected);
        QCOMPARE(source(reopened, "a"), QString("b1")); QVERIFY(!reopened.dirty());
        reopened.end();
    }
    void guidedWaitsForReleaseAndCancellationDoesNotWrite() {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        QVERIFY(index >= 0); auto* joystick = SDL_JoystickOpen(index); QVERIFY(joystick);
        const auto cleanup = qScopeGuard([joystick, index] { SDL_JoystickClose(joystick); SDL_JoystickDetachVirtual(index); });
        QTemporaryDir data; GamepadMapper mapper(data.path()); mapper.begin();
        for (int i = 0; i < mapper.devices().size(); ++i)
            if (mapper.devices()[i].toMap()["index"].toInt() == index) mapper.selectDevice(i);
        mapper.startGuided(); mapper.poll();
        QCOMPARE(mapper.target(), QString("dpup"));
        SDL_JoystickSetVirtualButton(joystick, 0, 1); mapper.poll();
        QCOMPARE(mapper.progress(), 1); QCOMPARE(mapper.target(), QString("dpdown"));
        for (int i = 0; i < 10; ++i) mapper.poll();
        QCOMPARE(mapper.progress(), 1);
        SDL_JoystickSetVirtualButton(joystick, 0, 0); mapper.poll();
        SDL_JoystickSetVirtualButton(joystick, 2, 1); mapper.poll();
        QCOMPARE(mapper.progress(), 2);
        mapper.stopCapture(); QVERIFY(mapper.target().isEmpty()); QVERIFY(!mapper.guided());
        mapper.capture("not-a-control"); QVERIFY(mapper.target().isEmpty());
        mapper.end(); QVERIFY(GamepadMapper::launchArguments(data.path()).isEmpty());
        mapper.begin(); mapper.startGuided();
        for (int i = 0; i < 25; ++i) mapper.skip();
        QVERIFY(!mapper.guided()); QCOMPARE(mapper.progress(), 25);
        mapper.end();
    }
    void disconnectStopsCaptureAndKeepsSavedMap() {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        QVERIFY(index >= 0);
        QTemporaryDir data; GamepadMapper mapper(data.path()); mapper.begin();
        for (int i = 0; i < mapper.devices().size(); ++i)
            if (mapper.devices()[i].toMap()["index"].toInt() == index) mapper.selectDevice(i);
        mapper.startGuided(); QVERIFY(!mapper.target().isEmpty());
        QVERIFY(SDL_JoystickDetachVirtual(index) == 0); mapper.poll();
        QVERIFY(mapper.target().isEmpty()); QVERIFY(!mapper.guided());
        QVERIFY(!mapper.save()); QVERIFY(GamepadMapper::launchArguments(data.path()).isEmpty());
    }
    void restoringDefaultsRemovesTheOverride() {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        QVERIFY(index >= 0); auto* joystick = SDL_JoystickOpen(index); QVERIFY(joystick);
        const auto cleanup = qScopeGuard([joystick, index] { SDL_JoystickClose(joystick); SDL_JoystickDetachVirtual(index); });
        QTemporaryDir data; GamepadMapper mapper(data.path()); mapper.begin();
        for (int i = 0; i < mapper.devices().size(); ++i)
            if (mapper.devices()[i].toMap()["index"].toInt() == index) mapper.selectDevice(i);
        const auto original = source(mapper, "a");
        mapper.capture("a"); mapper.poll();
        SDL_JoystickSetVirtualButton(joystick, 15, 1); mapper.poll();
        QVERIFY(source(mapper, "a") != original); QVERIFY(mapper.save());
        QVERIFY(!GamepadMapper::launchArguments(data.path()).isEmpty());
        mapper.restoreDefaults(); QCOMPARE(source(mapper, "a"), original);
        QVERIFY(mapper.save()); QVERIFY(GamepadMapper::launchArguments(data.path()).isEmpty());
        mapper.end();
    }
};
QTEST_GUILESS_MAIN(GamepadMapperTests)
#include "gamepad_mapper_tests.moc"
