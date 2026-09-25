#pragma once

#include "Constants.h"
#include "PrismBridge.h"
#include "ThemeManager.h"

#include <QWizard>

class QLabel;
class QLineEdit;
class QPushButton;
class QListWidget;

// First-run setup: theme selection → Prism path validation → accounts.
// Finishes only when a native (non-Flatpak) Prism binary + instances dir is
// confirmed; persists theme + paths via ThemeManager.
class SetupWizard : public QWizard {
    Q_OBJECT

public:
    SetupWizard(PrismBridge* bridge, ThemeManager* themes, QWidget* parent = nullptr);

    ThemeManager::Theme chosenTheme() const { return m_chosenTheme; }
    QString chosenBinaryPath() const { return m_validated.binaryPath; }
    QString chosenInstancesDir() const { return m_validated.instancesDir; }

private:
    QWizardPage* createThemePage();
    QWizardPage* createPathsPage();
    QWizardPage* createAccountsPage();
    void runValidation();
    void applyValidation(const EnvValidation& result);

    PrismBridge* m_bridge;
    ThemeManager* m_themes;
    ThemeManager::Theme m_chosenTheme = ThemeManager::Theme::Dark;
    EnvValidation m_validated;

    // Theme page
    QPushButton* m_darkButton = nullptr;
    QPushButton* m_lightButton = nullptr;

    // Paths page
    QLineEdit* m_binaryEdit = nullptr;
    QLineEdit* m_instancesEdit = nullptr;
    QLabel* m_validationResult = nullptr;
    QPushButton* m_validateButton = nullptr;

    // Accounts page
    QListWidget* m_accountList = nullptr;
    QPushButton* m_openPrismButton = nullptr;
};
