#include "PrismBridge.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

namespace {

constexpr char PRISM_BINARY_NAME[] = "prismlauncher";
constexpr int RUNNING_LOG_MAX_AGE_SEC = 90;

QString expandPath(const QString& raw) {
    QString path = raw;
    if (path.startsWith(QLatin1Char('~'))) {
        path = QDir::homePath() + path.mid(1);
    }
    return path;
}

bool isExecutableFile(const QString& path) {
    const QFileInfo info(path);
    return info.isFile() && info.isExecutable();
}

QString loaderChipName(const QString& uid) {
    if (uid.contains(QLatin1String("fabric"), Qt::CaseInsensitive)) {
        return QLatin1String("Fabric");
    }
    if (uid.contains(QLatin1String("neoforge"), Qt::CaseInsensitive)) {
        return QLatin1String("NeoForge");
    }
    if (uid.contains(QLatin1String("quilt"), Qt::CaseInsensitive)) {
        return QLatin1String("Quilt");
    }
    if (uid.contains(QLatin1String("forge"), Qt::CaseInsensitive)) {
        return QLatin1String("Forge");
    }
    return QString();
}

QJsonObject readJson(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object();
}

QString joinArgs(const QStringList& args) {
    return args.join(QLatin1Char(' '));
}

}  // namespace

// LogTail tails one instance log file via QFileSystemWatcher and emits the
// new lines through PrismBridge. Handles missing files (bounded retry) and
// rotation (directory watch re-arms the file watch).
class LogTail : public QObject {
public:
    explicit LogTail(PrismBridge* bridge)
        : QObject(bridge), m_bridge(bridge) {}

    ~LogTail() override { stop(); }

    bool start(const QString& path) {
        stop();
        m_path = path;
        m_offset = 0;

        if (path.isEmpty()) {
            emit m_bridge->logStatus(QLatin1String(Constants::MSG_NO_LOG));
            return false;
        }

        if (!QFileInfo::exists(path)) {
            emit m_bridge->logStatus(QLatin1String(Constants::MSG_WAITING_LOG));
            m_retries = 0;
            m_retryTimer = new QTimer(this);
            m_retryTimer->setInterval(Constants::LOG_TAIL_POLL_MS);
            connect(m_retryTimer, &QTimer::timeout, this, [this]() {
                if (QFileInfo::exists(m_path)) {
                    m_retryTimer->stop();
                    m_retryTimer->deleteLater();
                    m_retryTimer = nullptr;
                    openAndRead();
                } else if (++m_retries > 50) {
                    emit m_bridge->logStatus(QLatin1String(Constants::MSG_NO_LOG));
                    stop();
                }
            });
            m_retryTimer->start();
            return false;
        }

        return openAndRead();
    }

    void stop() {
        if (m_watcher) {
            m_watcher->deleteLater();
            m_watcher = nullptr;
        }
        if (m_retryTimer) {
            m_retryTimer->stop();
            m_retryTimer->deleteLater();
            m_retryTimer = nullptr;
        }
        m_offset = 0;
    }

private:
    bool openAndRead() {
        QFile file(m_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit m_bridge->logStatus(QLatin1String(Constants::MSG_NO_LOG));
            return false;
        }

        // Tail from the end, like `tail -f`. History is loaded by LogViewer.
        m_offset = file.size();

        if (!m_watcher) {
            m_watcher = new QFileSystemWatcher(this);
            connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this]() {
                if (!QFileInfo::exists(m_path)) {
                    emit m_bridge->logStatus(QLatin1String(Constants::MSG_WAITING_LOG));
                    return;
                }
                if (!m_watcher->files().contains(m_path)) {
                    // Rotated: the new file replaced the watched inode.
                    m_watcher->addPath(m_path);
                }
                readAvailable();
            });
        }
        m_watcher->addPath(m_path);

        emit m_bridge->logStatus(QLatin1String("Tailing ") + m_path);
        return true;
    }

    void readAvailable() {
        QFile file(m_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        if (file.size() < m_offset) {
            m_offset = 0;  // truncated or rotated
        }
        if (!file.seek(m_offset)) {
            return;
        }
        while (!file.atEnd()) {
            const QByteArray line = file.readLine();
            if (!line.isEmpty()) {
                emit m_bridge->logLineReady(QString::fromUtf8(line).trimmed());
            }
        }
        m_offset = file.pos();
    }

    PrismBridge* m_bridge;
    QString m_path;
    qint64 m_offset = 0;
    int m_retries = 0;
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_retryTimer = nullptr;
};

PrismBridge::PrismBridge(QObject* parent)
    : QObject(parent), m_logTail(new LogTail(this)) {}

void PrismBridge::setBinaryPath(const QString& path) {
    m_binaryPath = path;
}

void PrismBridge::setInstancesDir(const QString& path) {
    m_instancesDir = path;
}

QString PrismBridge::resolveBinary() const {
    if (!m_binaryPath.isEmpty()) {
        return isExecutableFile(m_binaryPath) ? m_binaryPath : QString();
    }
    for (const char* candidate : Constants::PRISM_BIN_CANDIDATES) {
        const QString path = expandPath(QString::fromUtf8(candidate));
        if (isExecutableFile(path)) {
            return path;
        }
    }
    return QStandardPaths::findExecutable(QLatin1String(PRISM_BINARY_NAME));
}

QString PrismBridge::resolveInstancesDir() const {
    if (!m_instancesDir.isEmpty()) {
        return QDir(m_instancesDir).exists() ? m_instancesDir : QString();
    }
    for (const char* candidate : Constants::PRISM_DATA_CANDIDATES) {
        const QDir root(expandPath(QString::fromUtf8(candidate)));
        const QString instances = root.filePath(QLatin1String("instances"));
        if (QDir(instances).exists()) {
            return instances;
        }
    }
    return QString();
}

EnvValidation PrismBridge::validateEnvironment() const {
    EnvValidation result;
    result.binaryPath = resolveBinary();
    result.instancesDir = resolveInstancesDir();

    // Flatpak detection: user sandbox dir or system-wide install. Flatpak is
    // unsupported — sandbox isolation blocks CLI hand-off and file monitoring.
    const QFileInfo userFlatpak(expandPath(QLatin1String("$HOME/.var/app/org.prismlauncher.PrismLauncher")));
    const QFileInfo systemFlatpak(QLatin1String("/var/lib/flatpak/app/org.prismlauncher.PrismLauncher"));
    if (userFlatpak.isDir() || systemFlatpak.isDir()) {
        result.flatpakDetected = true;
        result.error = QLatin1String(Constants::MSG_FLATPAK);
        result.detail = QLatin1String(
            "Flatpak sandboxing blocks the CLI hand-off and file monitoring Slime Launcher depends on. "
            "Install a native package instead:\n"
            "  • Arch Linux:    sudo pacman -S prismlauncher\n"
            "  • AUR:           paru -S prismlauncher-bin\n"
            "  • Debian/Ubuntu: sudo apt install prismlauncher\n"
            "  • Fedora:        sudo dnf install prismlauncher\n"
            "Then re-run Slime Launcher setup.");
        return result;
    }

    if (result.binaryPath.isEmpty()) {
        result.error = QLatin1String(Constants::MSG_NO_PRISM);
        result.detail = QLatin1String(
            "Searched /usr/bin, /usr/local/bin and your PATH for 'prismlauncher' and found nothing. "
            "Install Prism Launcher natively, then re-run setup.");
        return result;
    }

    if (result.instancesDir.isEmpty()) {
        result.error = QLatin1String(
            "Prism binary found, but no instances directory. Launch the native Prism Launcher once to create one.");
        result.detail = QLatin1String(
            "Searched ~/.local/share/PrismLauncher/instances, ~/.local/share/prism-launcher/instances, "
            "and ~/.local/share/PolyMC/instances.");
        return result;
    }

    result.ok = true;
    return result;
}

OpResult PrismBridge::verifyBinary() const {
    if (m_binaryPath.isEmpty()) {
        return OpResult::fail(QLatin1String(Constants::MSG_NO_PRISM));
    }
    if (!isExecutableFile(m_binaryPath)) {
        return OpResult::fail(QLatin1String("Configured Prism binary is missing or not executable: ") + m_binaryPath);
    }
    return OpResult::success();
}

QVector<InstanceCardModel> PrismBridge::scanInstances(QString* errorOut) const {
    QVector<InstanceCardModel> results;
    if (errorOut) {
        errorOut->clear();
    }

    const OpResult binaryCheck = verifyBinary();
    const QString dir = resolveInstancesDir();
    if (!binaryCheck.ok) {
        if (errorOut) {
            *errorOut = binaryCheck.error;
        }
        return results;
    }
    if (dir.isEmpty()) {
        if (errorOut) {
            *errorOut = QLatin1String("Instances directory is missing: ") + m_instancesDir;
        }
        return results;
    }

    const QDir root(dir);
    const QFileInfoList entries =
        root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& entry : entries) {
        InstanceCardModel info = parseInstanceDir(QDir(entry.absoluteFilePath()));
        if (info.id.isEmpty()) {
            continue;  // not a Prism instance directory
        }
        // Provision the Slime-owned asset folder: auto-create slimelauncher/,
        // seed a deterministic default poster when no card.png exists yet.
        info.cardPath = ImageProcessor::ensureAssets(dir, info);
        results.append(info);
    }

    std::sort(results.begin(), results.end(), [](const InstanceCardModel& a, const InstanceCardModel& b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    return results;
}

InstanceCardModel PrismBridge::parseInstanceDir(const QDir& dir) const {
    InstanceCardModel info;
    info.id = dir.dirName();

    // instance.cfg is mandatory — without it this is not a Prism instance.
    const QString cfgPath = dir.filePath(QLatin1String("instance.cfg"));
    if (!QFileInfo::exists(cfgPath)) {
        info.id.clear();
        return info;
    }

    QSettings cfg(cfgPath, QSettings::IniFormat);
    // QSettings maps [General] sections to "General/name" subkeys, while real
    // Prism files are flat — accept both spellings.
    info.name = cfg.value(QLatin1String("General/name")).toString();
    if (info.name.isEmpty()) {
        info.name = cfg.value(QLatin1String("name")).toString();
    }
    if (info.name.isEmpty()) {
        info.name = info.id;
    }
    QString iconKey = cfg.value(QLatin1String("General/iconKey")).toString();
    if (iconKey.isEmpty()) {
        iconKey = cfg.value(QLatin1String("iconKey")).toString();
    }
    info.iconPath = iconPathFor(iconKey);
    QString lastLaunch = cfg.value(QLatin1String("General/lastLaunchTime")).toString();
    if (lastLaunch.isEmpty()) {
        lastLaunch = cfg.value(QLatin1String("lastLaunchTime")).toString();
    }
    if (!lastLaunch.isEmpty()) {
        info.lastPlayed = QDateTime::fromString(lastLaunch, Qt::ISODateWithMs);
    }

    // mmc-pack.json carries the component list (loaders + game version).
    QString gameVersion;
    info.loader = loaderFromComponents(readJson(dir.filePath(QLatin1String("mmc-pack.json"))), &gameVersion);
    if (info.loader.isEmpty()) {
        info.loader = QLatin1String("Vanilla");
    }
    info.gameVersion = gameVersion;

    info.playing = isInstanceRunning(info);
    return info;
}

QString PrismBridge::loaderFromComponents(const QJsonObject& root, QString* gameVersion) const {
    const QJsonArray components = root.value(QLatin1String("components")).toArray();
    QString loader;
    for (const QJsonValue& v : components) {
        const QJsonObject obj = v.toObject();
        const QString uid = obj.value(QLatin1String("uid")).toString();
        if (uid == QLatin1String("net.minecraft")) {
            if (gameVersion && gameVersion->isEmpty()) {
                *gameVersion = obj.value(QLatin1String("version")).toString();
            }
            continue;
        }
        if (loader.isEmpty()) {
            loader = loaderChipName(uid);
        }
    }
    return loader;
}

QString PrismBridge::iconPathFor(const QString& iconKey) const {
    if (iconKey.isEmpty() || m_instancesDir.isEmpty()) {
        return QString();
    }
    // The icons directory is a sibling of the instances directory.
    const QDir root(QFileInfo(m_instancesDir).dir());
    static const char* kExtensions[] = {"png", "jpg", "jpeg", "gif", "svg", "ico"};
    for (const char* ext : kExtensions) {
        const QString path = root.filePath(QLatin1String("icons/%1.%2").arg(iconKey, QLatin1String(ext)));
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

bool PrismBridge::isInstanceRunning(const InstanceCardModel& info) const {
    // Heuristic: Prism keeps latest.log open while the game runs, so a recent
    // mtime means the game is (very likely) still running.
    const QString log = instanceLogPath(info.id);
    if (log.isEmpty()) {
        return false;
    }
    const QFileInfo logInfo(log);
    if (!logInfo.exists()) {
        return false;
    }
    return logInfo.lastModified().secsTo(QDateTime::currentDateTime()) < RUNNING_LOG_MAX_AGE_SEC;
}

QString PrismBridge::instanceLogPath(const QString& id) const {
    // Prism 9+ uses .minecraft/logs/latest.log; PolyMC-heritage layouts used
    // "minecraft" and legacy 1.log. Check all variants.
    static const char* kLogCandidates[] = {
        ".minecraft/logs/latest.log",
        "minecraft/logs/latest.log",
        ".minecraft/logs/1.log",
        "minecraft/logs/1.log",
    };
    const QDir instDir = QDir(m_instancesDir).filePath(id);
    for (const char* rel : kLogCandidates) {
        const QString path = instDir.filePath(QLatin1String(rel));
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

QVector<AccountInfo> PrismBridge::readAccounts() const {
    QVector<AccountInfo> accounts;
    if (m_instancesDir.isEmpty()) {
        return accounts;
    }
    // accounts.json sits in the Prism data root, next to instances/.
    const QString path = QFileInfo(m_instancesDir).dir().filePath(QLatin1String("accounts.json"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return accounts;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        const QJsonObject obj = v.toObject();
        AccountInfo acc;
        acc.name = obj.value(QLatin1String("name")).toString();
        const QString type = obj.value(QLatin1String("type")).toString();
        if (type == QLatin1String("MSA")) {
            acc.type = QLatin1String("Microsoft");
        } else if (type == QLatin1String("local")) {
            acc.type = QLatin1String("Local");
        } else if (type == QLatin1String("offline")) {
            acc.type = QLatin1String("Offline");
        } else {
            acc.type = type;
        }
        acc.lastSync = obj.value(QLatin1String("lastSync")).toString();
        accounts.append(acc);
    }
    return accounts;
}

OpResult PrismBridge::setInstanceCardArtwork(const QString& instanceId, const QString& sourceImagePath) {
    if (m_instancesDir.isEmpty()) {
        return OpResult::fail(QLatin1String("Instances directory is not configured."));
    }
    if (instanceId.contains(QLatin1Char('/')) || instanceId.contains(QLatin1String(".."))) {
        return OpResult::fail(QLatin1String("Invalid instance id."));
    }
    QString error;
    if (!ImageProcessor::importCardArtwork(m_instancesDir, instanceId, sourceImagePath, &error)) {
        return OpResult::fail(error.isEmpty() ? QLatin1String(Constants::MSG_ARTWORK_FAILED) : error);
    }
    return OpResult::success();
}

OpResult PrismBridge::launchInstance(const QString& id) {
    const OpResult binaryCheck = verifyBinary();
    if (!binaryCheck.ok) {
        return binaryCheck;
    }
    if (id.trimmed().isEmpty()) {
        return OpResult::fail(QLatin1String("Instance id is empty — nothing to launch."));
    }

    // All launch operations route through Prism's standard CLI backend.
    const QStringList args = {QLatin1String("--launch"), id};
    const qint64 pid = QProcess::startDetached(m_binaryPath, args);
    if (pid == 0) {
        return OpResult::fail(QLatin1String(Constants::MSG_SPAWN_FAIL)
                              + QLatin1String(" Command: ") + m_binaryPath + QLatin1Char(' ')
                              + joinArgs(args));
    }
    return OpResult::success();
}

OpResult PrismBridge::openPrismUi() {
    const OpResult binaryCheck = verifyBinary();
    if (!binaryCheck.ok) {
        return binaryCheck;
    }
    const qint64 pid = QProcess::startDetached(m_binaryPath, QStringList());
    if (pid == 0) {
        return OpResult::fail(QLatin1String(Constants::MSG_SPAWN_FAIL)
                              + QLatin1String(" Command: ") + m_binaryPath);
    }
    return OpResult::success();
}

bool PrismBridge::tailLog(const QString& instanceId) {
    return m_logTail && m_logTail->start(instanceLogPath(instanceId));
}

void PrismBridge::stopTailing() {
    if (m_logTail) {
        m_logTail->stop();
    }
}
