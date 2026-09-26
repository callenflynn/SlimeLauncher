#pragma once

#include "PrismBridge.h"
#include "ThemeManager.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QListWidget;

// Console Settings page (reached via LB/RB view cycling): theme toggle,
// active-account readout, and the Prism fallback actions. Deliberately
// minimal — account management itself always delegates to Prism.
class SettingsView : public QWidget {
    Q_OBJECT

public:
    SettingsView(PrismBridge* bridge, ThemeManager* themes, QWidget* parent = nullptr);

    void refreshAccountChip();

signals:
    void themeToggled(ThemeManager::Theme theme);
    void openPrismRequested();
    void refreshRequested();

private:
    void updateThemeButton();

    PrismBridge* m_bridge;
    ThemeManager* m_themes;
    QLabel* m_accountLabel = nullptr;
    QPushButton* m_themeButton = nullptr;
};
