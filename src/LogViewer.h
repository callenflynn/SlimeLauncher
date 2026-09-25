#pragma once

#include "Constants.h"

#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QLabel;
class PrismBridge;

// Read-only instance log panel. Streams lines from PrismBridge::tailLog into
// a QPlainTextEdit with follow-mode (auto-scroll pauses on manual scroll-up).
class LogViewer : public QWidget {
    Q_OBJECT

public:
    explicit LogViewer(PrismBridge* bridge, QWidget* parent = nullptr);

    void attachToInstance(const QString& instanceId);
    void detach();

signals:
    void closeRequested();

private:
    void appendLine(const QString& line);
    void setStatus(const QString& message);

    PrismBridge* m_bridge;
    QString m_instanceId;
    QPlainTextEdit* m_view = nullptr;
    QPushButton* m_followButton = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    bool m_follow = true;
};
