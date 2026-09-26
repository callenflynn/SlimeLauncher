#include "SettingsView.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SettingsView::SettingsView(PrismBridge* bridge, ThemeManager* themes, QWidget* parent)
    : QWidget(parent), m_bridge(bridge), m_themes(themes) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(64, 48, 64, 48);
    layout->setSpacing(14);

    auto* title = new QLabel(QLatin1String("SETTINGS"), this);
    title->setStyleSheet(QStringLiteral(
        "font-size: 22px; font-weight: 800; letter-spacing: 6px; color: #00f0ff;"));

    m_accountLabel = new QLabel(this);
    m_accountLabel->setWordWrap(true);
    m_accountLabel->setStyleSheet(QStringLiteral(
        "background: #1a1a24; border: 1px solid #2a2a38; border-radius: 12px;"
        " padding: 16px; font-size: 14px;"));

    m_themeButton = new QPushButton(this);
    m_themeButton->setMinimumHeight(44);
    connect(m_themeButton, &QPushButton::clicked, this, [this]() {
        const ThemeManager::Theme next = m_themes->theme() == ThemeManager::Theme::Dark
                                             ? ThemeManager::Theme::Light
                                             : ThemeManager::Theme::Dark;
        m_themes->setTheme(next);
        updateThemeButton();
        emit themeToggled(next);
    });

    auto* openPrism = new QPushButton(QLatin1String("Open Prism Launcher"), this);
    openPrism->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));
    openPrism->setMinimumHeight(44);
    connect(openPrism, &QPushButton::clicked, this, &SettingsView::openPrismRequested);

    auto* refresh = new QPushButton(QLatin1String("Rescan Library"), this);
    refresh->setMinimumHeight(44);
    connect(refresh, &QPushButton::clicked, this, &SettingsView::refreshRequested);

    auto* note = new QLabel(QLatin1String(
        "Account sign-in and session management are handled by Prism Launcher. "
        "Slime Launcher reads the account list read-only."), this);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: #9a9eb0; font-size: 12px;"));

    layout->addWidget(title);
    layout->addSpacing(8);
    layout->addWidget(m_accountLabel);
    layout->addWidget(m_themeButton);
    layout->addWidget(openPrism);
    layout->addWidget(refresh);
    layout->addSpacing(6);
    layout->addWidget(note);
    layout->addStretch(1);

    updateThemeButton();
    refreshAccountChip();
}

void SettingsView::updateThemeButton() {
    const bool dark = m_themes->theme() == ThemeManager::Theme::Dark;
    m_themeButton->setText(dark ? QLatin1String("Theme: Dark  (switch to Light)")
                                : QLatin1String("Theme: Light  (switch to Dark)"));
}

void SettingsView::refreshAccountChip() {
    const QVector<AccountInfo> accounts = m_bridge->readAccounts();
    if (accounts.isEmpty()) {
        m_accountLabel->setText(QLatin1String("No accounts detected.\nSign in through Prism Launcher."));
        return;
    }
    const AccountInfo& active = accounts.first();
    QString text;
    if (active.active) {
        text = QStringLiteral("Logged in as %1  ·  %2").arg(active.name, active.type);
    } else {
        text = QStringLiteral("Account: %1  ·  %2").arg(active.name, active.type);
    }
    if (!active.ownsMinecraft && active.type == QLatin1String("Microsoft")) {
        text += QStringLiteral("\nEntitlement: Minecraft ownership not detected.");
    }
    m_accountLabel->setText(text);
}
