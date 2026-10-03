#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    QFile record(qEnvironmentVariable("EMULOS_STUB_OUTPUT"));
    if (record.open(QIODevice::WriteOnly))
        record.write(QJsonDocument(QJsonObject{{"args", QJsonArray::fromStringList(args)},
            {"cwd", QDir::currentPath()}}).toJson());
    for (const auto& arg : args) if (arg.startsWith("--emulos_profile_output=")) {
        QFile output(arg.mid(QStringLiteral("--emulos_profile_output=").size()));
        if (!output.open(QIODevice::WriteOnly)) return 2;
        output.write("version = 1\nok = true\n[[profiles]]\nxuid = 'E000000000000001'\ngamertag = 'CoreTest'\nnetplay = true\n");
    }
    return 0;
}
