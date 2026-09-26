#pragma once

#include "Constants.h"
#include "InstanceCardModel.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

#include "ImageProcessor.h"

struct OpResult {
    bool ok = false;
    QString error;
    static OpResult success() { return {true, QString()}; }
    static OpResult fail(const QString& why) { return {false, why}; }
};

struct AccountInfo {
    QString name;
    QString type;
    QString lastSync;
    bool active = false;          // Prism marks the in-use session "active": true
    bool ownsMinecraft = false;   // entitlement.ownsMinecraft (MSA accounts)
};

struct EnvValidation {
    bool ok = false;
    bool flatpakDetected = false;
    QString binaryPath;
    QString instancesDir;
    QString error;
    QString detail;
};

Q_DECLARE_METATYPE(OpResult)
Q_DECLARE_METATYPE(AccountInfo)
Q_DECLARE_METATYPE(EnvValidation)

class LogTail;

// Single gateway to Prism on-disk data and processes. Prism is the source of
// truth; this class only reads Prism data and spawns Prism processes.
class PrismBridge : public QObject {
    Q_OBJECT

public:
    explicit PrismBridge(QObject* parent = nullptr);

    void setBinaryPath(const QString& path);
    void setInstancesDir(const QString& path);
    QString binaryPath() const { return m_binaryPath; }
    QString instancesDir() const { return m_instancesDir; }

    // Environment checks
    EnvValidation validateEnvironment() const;
    OpResult verifyBinary() const;

    // Instance data (read-only); scanInstances also provisions the
    // Slime-owned slimelauncher/ asset folder per instance.
    QVector<InstanceCardModel> scanInstances(QString* errorOut) const;
    QString instanceLogPath(const QString& id) const;
    QVector<AccountInfo> readAccounts() const;
    bool isInstanceRunning(const InstanceCardModel& info) const;

    // Slime asset pipeline: runs a user-picked image through the 2:3 crop
    // engine and saves it as the instance's card.png. The only file Slime
    // writes inside an instance directory (under slimelauncher/).
    OpResult setInstanceCardArtwork(const QString& instanceId, const QString& sourceImagePath);

    // Process operations (async)
    OpResult launchInstance(const QString& id);
    OpResult openPrismUi();

    // Log tailing
    bool tailLog(const QString& instanceId);
    void stopTailing();

signals:
    void logLineReady(const QString& text);
    void logStatus(const QString& message);

private:
    QString resolveBinary() const;
    QString resolveInstancesDir() const;
    InstanceCardModel parseInstanceDir(const QDir& dir) const;
    QString loaderFromComponents(const QJsonObject& root, QString* gameVersion) const;
    QString iconPathFor(const QString& iconKey) const;
    QString resolveAccountsPath() const;
    static void parseAccountsDocument(const QJsonDocument& doc, QVector<AccountInfo>* out);

    QString m_binaryPath;
    QString m_instancesDir;
    LogTail* m_logTail = nullptr;
};
