// Headless integration smoke test: drives PrismBridge against a fake Prism
// environment to verify environment validation, instance parsing, accounts,
// log tailing, CLI launch routing, and the slimelauncher/ image pipeline.
// Not linked into the shipped binary.
#include "PrismBridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QTimer>

#include <cstdio>

namespace {

bool cardHasPostageStamp(const QString& path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) {
        std::fprintf(stderr, "FAIL: cannot decode card asset %s\n", path.toUtf8().constData());
        return false;
    }
    // 2:3 window: min 300x450, max 600x900.
    if (img.width() < 300 || img.height() < 450 || img.width() > 600 || img.height() > 900 ||
        img.width() * 3 != img.height() * 2) {
        std::fprintf(stderr, "FAIL: card asset %s is %dx%d, outside the 2:3 window\n",
                     path.toUtf8().constData(), img.width(), img.height());
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QString home = qEnvironmentVariable("SMOKE_HOME");
    if (home.isEmpty()) {
        std::fprintf(stderr, "SMOKE_HOME not set\n");
        return 2;
    }

    PrismBridge bridge;

    // 1. Empty environment → must fail with MSG_NO_PRISM.
    {
        const EnvValidation v = bridge.validateEnvironment();
        std::printf("empty-env ok=%d flatpak=%d err=%s\n", v.ok ? 1 : 0, v.flatpakDetected ? 1 : 0,
                    v.error.toUtf8().constData());
        if (v.ok || v.error.isEmpty()) {
            std::fprintf(stderr, "FAIL: expected validation failure on empty env\n");
            return 1;
        }
    }

    // 2. Point the bridge at the fake install.
    bridge.setBinaryPath(home + "/bin/prismlauncher");
    bridge.setInstancesDir(home + "/data/instances");

    // 3. Validation must now pass.
    {
        const EnvValidation v = bridge.validateEnvironment();
        std::printf("fake-env ok=%d bin=%s inst=%s\n", v.ok ? 1 : 0,
                    v.binaryPath.toUtf8().constData(), v.instancesDir.toUtf8().constData());
        if (!v.ok) {
            std::fprintf(stderr, "FAIL: expected validation success on fake env\n");
            return 1;
        }
    }

    // 4. Instance scan must find creative-world with Fabric loader + version.
    {
        QString error;
        const QVector<InstanceCardModel> instances = bridge.scanInstances(&error);
        std::printf("scan count=%lld error=%s\n", static_cast<long long>(instances.size()), error.toUtf8().constData());
        if (!error.isEmpty() || instances.size() != 1) {
            std::fprintf(stderr, "FAIL: expected exactly 1 instance\n");
            return 1;
        }
        const InstanceCardModel& info = instances.first();
        std::printf("instance id=%s name=%s loader=%s ver=%s\n", info.id.toUtf8().constData(),
                    info.name.toUtf8().constData(), info.loader.toUtf8().constData(),
                    info.gameVersion.toUtf8().constData());
        if (info.id != QLatin1String("creative-world") || info.name != QLatin1String("Creative World") ||
            info.loader != QLatin1String("Fabric") || info.gameVersion != QLatin1String("1.21.4")) {
            std::fprintf(stderr, "FAIL: instance fields mismatch\n");
            return 1;
        }
    }

    // 4b. Asset pipeline: slimelauncher/ folder must exist with a valid
    // 2:3 card.png and a metadata.json marking the default artwork.
    {
        const QString slimeDir = home + "/data/instances/creative-world/slimelauncher";
        const QString card = slimeDir + "/card.png";
        const QString metadata = slimeDir + "/metadata.json";
        if (!QDir(slimeDir).exists() || !QFileInfo::exists(card) || !QFileInfo::exists(metadata)) {
            std::fprintf(stderr, "FAIL: slimelauncher/ assets were not provisioned\n");
            return 1;
        }
        if (!cardHasPostageStamp(card)) {
            return 1;
        }
        QFile metaFile(metadata);
        if (!metaFile.open(QIODevice::ReadOnly) ||
            !QString::fromUtf8(metaFile.readAll()).contains("\"artwork\": \"default\"")) {
            std::fprintf(stderr, "FAIL: metadata.json missing default artwork marker\n");
            return 1;
        }
    }

    // 4c. Custom artwork import: a wide 800x500 image must land as a 2:3 card.
    {
        QImage wide(800, 500, QImage::Format_ARGB32);
        wide.fill(0x3366aa);
        const QString source = home + "/wide-art.png";
        wide.save(source, "PNG");
        const OpResult r = bridge.setInstanceCardArtwork("creative-world", source);
        if (!r.ok) {
            std::fprintf(stderr, "FAIL: artwork import failed: %s\n", r.error.toUtf8().constData());
            return 1;
        }
        if (!cardHasPostageStamp(home + "/data/instances/creative-world/slimelauncher/card.png")) {
            return 1;
        }
        QFile metaFile(home + "/data/instances/creative-world/slimelauncher/metadata.json");
        if (!metaFile.open(QIODevice::ReadOnly) ||
            !QString::fromUtf8(metaFile.readAll()).contains("\"artwork\": \"custom\"")) {
            std::fprintf(stderr, "FAIL: metadata.json missing custom artwork marker\n");
            return 1;
        }
    }

    // 5. Accounts read (empty file → empty list, no crash).
    {
        const QVector<AccountInfo> accounts = bridge.readAccounts();
        std::printf("accounts=%lld\n", static_cast<long long>(accounts.size()));
    }

    // 6. Launch routing: stub records its argv; verify the --launch verb.
    {
        const OpResult r = bridge.launchInstance("creative-world");
        std::printf("launch ok=%d err=%s\n", r.ok ? 1 : 0, r.error.toUtf8().constData());

        // The detached stub needs a moment to record its argv.
        QEventLoop wait;
        QTimer::singleShot(500, &wait, &QEventLoop::quit);
        wait.exec();

        QFile argvFile(home + "/argv.txt");
        if (!r.ok || !argvFile.open(QIODevice::ReadOnly)) {
            std::fprintf(stderr, "FAIL: launch did not route through CLI\n");
            return 1;
        }
        const QString argv = QString::fromUtf8(argvFile.readAll()).simplified();
        std::printf("cli=%s\n", argv.toUtf8().constData());
        if (!argv.contains("--launch creative-world")) {
            std::fprintf(stderr, "FAIL: wrong CLI verb\n");
            return 1;
        }
    }

    // 7. Log tail: append a line, expect a signal within 2s.
    {
        QString lastLine;
        QEventLoop loop;
        QObject::connect(&bridge, &PrismBridge::logLineReady, &loop, [&](const QString& line) {
            lastLine = line;
            loop.quit();
        });
        bridge.tailLog("creative-world");
        QTimer::singleShot(300, [&]() {
            QFile log(home + "/data/instances/creative-world/.minecraft/logs/latest.log");
            if (log.open(QIODevice::Append)) {
                log.write("[10:30:05] [main/INFO]: smoke-test append\n");
            }
        });
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();
        bridge.stopTailing();
        std::printf("tail=%s\n", lastLine.toUtf8().constData());
        if (!lastLine.contains("smoke-test append")) {
            std::fprintf(stderr, "FAIL: log tail did not deliver appended line\n");
            return 1;
        }
        bridge.stopTailing();
    }

    std::printf("ALL CHECKS PASSED\n");
    return 0;
}
