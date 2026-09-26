#include "SetupWizard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWizardPage>

SetupWizard::SetupWizard(PrismBridge* bridge, ThemeManager* themes, QWidget* parent)
    : QWizard(parent), m_bridge(bridge), m_themes(themes) {
    setWindowTitle(QLatin1String(Constants::APP_NAME) + QStringLiteral(" — Setup"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setFixedSize(760, 560);

    addPage(createThemePage());
    addPage(createPathsPage());
    addPage(createAccountsPage());
}

QWizardPage* SetupWizard::createThemePage() {
    auto* page = new QWizardPage(this);
    page->setTitle(QLatin1String("Choose your theme"));

    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(14);

    auto* intro = new QLabel(
        QLatin1String("Dark is the default: deep charcoal with electric-cyan focus glows. "
                      "Light mode is a full-contrast alternative. You can change this later."),
        page);
    intro->setWordWrap(true);

    auto* row = new QHBoxLayout();
    m_darkButton = new QPushButton(QLatin1String("DARK\n#0f0f13 base\nneon #00f0ff accents"), page);
    m_darkButton->setMinimumSize(220, 140);
    m_darkButton->setCheckable(true);
    m_darkButton->setChecked(true);
    m_darkButton->setStyleSheet(
        QStringLiteral("QPushButton { background: #0f0f13; color: #f2f4f8; border: 3px solid #00f0ff; "
                       "border-radius: 12px; font-weight: 800; text-align: center; padding: 12px; }"));

    m_lightButton = new QPushButton(QLatin1String("LIGHT\n#f4f5f9 base\nfull contrast"), page);
    m_lightButton->setMinimumSize(220, 140);
    m_lightButton->setCheckable(true);
    m_lightButton->setStyleSheet(
        QStringLiteral("QPushButton { background: #f4f5f9; color: #15161c; border: 3px solid #d5d7e2; "
                       "border-radius: 12px; font-weight: 800; text-align: center; padding: 12px; }"
                       "QPushButton:checked { border-color: #6a1fb8; }"));

    row->addWidget(m_darkButton);
    row->addWidget(m_lightButton);
    row->addStretch(1);

    layout->addWidget(intro);
    layout->addLayout(row);
    layout->addStretch(1);

    connect(m_darkButton, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            m_lightButton->setChecked(false);
            m_chosenTheme = ThemeManager::Theme::Dark;
            m_themes->setTheme(ThemeManager::Theme::Dark);
        }
    });
    connect(m_lightButton, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            m_darkButton->setChecked(false);
            m_chosenTheme = ThemeManager::Theme::Light;
            m_themes->setTheme(ThemeManager::Theme::Light);
        }
    });

    return page;
}

QWizardPage* SetupWizard::createPathsPage() {
    auto* page = new QWizardPage(this);
    page->setTitle(QLatin1String("Prism Launcher installation"));

    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(12);

    auto* intro = new QLabel(
        QLatin1String("Slime Launcher needs a native (non-Flatpak) Prism Launcher v9.0+ install. "
                      "Standard paths are scanned automatically; adjust below for custom installs."),
        page);
    intro->setWordWrap(true);

    auto* binaryRow = new QHBoxLayout();
    binaryRow->addWidget(new QLabel(QLatin1String("Binary"), page));
    m_binaryEdit = new QLineEdit(page);
    m_binaryEdit->setPlaceholderText(QLatin1String("/usr/bin/prismlauncher"));
    binaryRow->addWidget(m_binaryEdit, 1);

    auto* instancesRow = new QHBoxLayout();
    instancesRow->addWidget(new QLabel(QLatin1String("Instances dir"), page));
    m_instancesEdit = new QLineEdit(page);
    m_instancesEdit->setPlaceholderText(
        QLatin1String("~/.local/share/PrismLauncher/instances"));
    instancesRow->addWidget(m_instancesEdit, 1);

    m_validateButton = new QPushButton(QLatin1String("Validate"), page);
    m_validateButton->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));

    m_validationResult = new QLabel(page);
    m_validationResult->setWordWrap(true);
    m_validationResult->setStyleSheet(QStringLiteral("font-size: 12px;"));

    layout->addWidget(intro);
    layout->addLayout(binaryRow);
    layout->addLayout(instancesRow);
    layout->addWidget(m_validateButton);
    layout->addWidget(m_validationResult);
    layout->addStretch(1);

    connect(m_validateButton, &QPushButton::clicked, this, &SetupWizard::runValidation);

    return page;
}

QWizardPage* SetupWizard::createAccountsPage() {
    auto* page = new QWizardPage(this);
    page->setTitle(QLatin1String("Accounts"));

    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(12);

    auto* intro = new QLabel(
        QLatin1String("Prism manages account credentials. Slime Launcher reads the account list "
                      "read-only — to sign in with a Microsoft account or manage sessions, open the "
                      "native Prism UI."),
        page);
    intro->setWordWrap(true);

    m_accountList = new QListWidget(page);
    m_accountList->setSelectionMode(QAbstractItemView::NoSelection);
    m_accountList->setFixedHeight(180);

    m_openPrismButton = new QPushButton(QLatin1String("Open Prism to sign in / manage accounts"), page);
    m_openPrismButton->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));

    layout->addWidget(intro);
    layout->addWidget(m_accountList);
    layout->addWidget(m_openPrismButton);
    layout->addStretch(1);

    connect(m_openPrismButton, &QPushButton::clicked, this, [this]() {
        m_bridge->openPrismUi();
    });

    return page;
}

void SetupWizard::runValidation() {
    // Manual entries win; empty fields fall back to auto-scan.
    const QString manualBinary = m_binaryEdit->text().trimmed();
    const QString manualInstances = m_instancesEdit->text().trimmed();
    if (!manualBinary.isEmpty()) {
        m_bridge->setBinaryPath(manualBinary);
    }
    if (!manualInstances.isEmpty()) {
        m_bridge->setInstancesDir(manualInstances);
    }
    applyValidation(m_bridge->validateEnvironment());
}

void SetupWizard::applyValidation(const EnvValidation& result) {
    m_validated = result;
    if (result.ok) {
        m_validationResult->setStyleSheet(QStringLiteral("color: #00f0ff; font-size: 12px; font-weight: 700;"));
        m_validationResult->setText(QStringLiteral("OK  •  %1\n%2").arg(result.binaryPath, result.instancesDir));
        return;
    }
    if (result.flatpakDetected) {
        m_validationResult->setStyleSheet(QStringLiteral("color: #ff4d6a; font-size: 12px;"));
    } else {
        m_validationResult->setStyleSheet(QStringLiteral("color: #ff9f1c; font-size: 12px;"));
    }
    m_validationResult->setText(result.error + QStringLiteral("\n\n") + result.detail);
}
