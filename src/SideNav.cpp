#include "SideNav.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SideNav::SideNav(PrismBridge* bridge, QWidget* parent)
    : QWidget(parent), m_bridge(bridge) {
    setObjectName(QLatin1String(Constants::OBJ_SIDE_NAV));
    setFixedWidth(200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 18, 14, 18);
    layout->setSpacing(8);

    auto* brand = new QLabel(QLatin1String("SLIME"), this);
    brand->setStyleSheet(
        QStringLiteral("font-weight: 900; font-size: 20px; letter-spacing: 4px; color: #39ff14;"));
    auto* subtitle = new QLabel(QLatin1String("LAUNCHER"), this);
    subtitle->setStyleSheet(QStringLiteral("font-size: 10px; letter-spacing: 6px;"));

    m_accountChip = new QLabel(this);
    m_accountChip->setWordWrap(true);
    m_accountChip->setStyleSheet(
        QStringLiteral("background: #141414; border: 1px solid #262626; padding: 8px; font-size: 11px;"));

    m_allButton = new QPushButton(QLatin1String("All Instances"), this);
    m_refreshButton = new QPushButton(QLatin1String("Refresh (F5)"), this);
    m_prismButton = new QPushButton(QLatin1String("Open Prism (O)"), this);
    m_prismButton->setObjectName(QLatin1String(Constants::OBJ_BUTTON));

    layout->addWidget(brand);
    layout->addWidget(subtitle);
    layout->addSpacing(12);
    layout->addWidget(m_allButton);
    layout->addWidget(m_refreshButton);
    layout->addWidget(m_prismButton);
    layout->addStretch(1);
    layout->addWidget(m_accountChip);

    connect(m_allButton, &QPushButton::clicked, this, &SideNav::allRequested);
    connect(m_refreshButton, &QPushButton::clicked, this, &SideNav::refreshRequested);
    connect(m_prismButton, &QPushButton::clicked, this, &SideNav::openPrismRequested);

    refreshAccountChip();
}

void SideNav::refreshAccountChip() {
    const QVector<AccountInfo> accounts = m_bridge->readAccounts();
    if (accounts.isEmpty()) {
        m_accountChip->setText(QLatin1String("No accounts.\nSign in via Prism."));
        return;
    }
    const AccountInfo& primary = accounts.first();
    m_accountChip->setText(QStringLiteral("%1\n%2").arg(primary.name, primary.type));
}
