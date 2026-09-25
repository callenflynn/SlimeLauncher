#pragma once

#include "Constants.h"
#include "PrismBridge.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

// Full-screen failure surface for fatal paths: no Prism install, no instances
// dir, spawn failure. Shows error text + remediation detail + Retry/Open Prism.
class ErrorPanel : public QWidget {
    Q_OBJECT

public:
    explicit ErrorPanel(QWidget* parent = nullptr);

    void showError(const QString& error, const QString& detail);

signals:
    void retryRequested();
    void openPrismRequested();

private:
    QLabel* m_errorTitle = nullptr;
    QLabel* m_errorDetail = nullptr;
    QPushButton* m_retryButton = nullptr;
    QPushButton* m_prismButton = nullptr;
    QVBoxLayout* m_layout = nullptr;
};
