#pragma once

#include "Constants.h"
#include "PrismBridge.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

// Left rail: All / Refresh / Open Prism buttons + read-only account chip.
// Fully keyboard/gamepad navigable via Qt's shared focus model.
class SideNav : public QWidget {
    Q_OBJECT

public:
    explicit SideNav(PrismBridge* bridge, QWidget* parent = nullptr);

    void refreshAccountChip();

signals:
    void allRequested();
    void refreshRequested();
    void openPrismRequested();

private:
    PrismBridge* m_bridge;
    QLabel* m_accountChip = nullptr;
    QPushButton* m_allButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_prismButton = nullptr;
};
