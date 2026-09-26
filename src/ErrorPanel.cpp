#include "ErrorPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ErrorPanel::ErrorPanel(QWidget* parent)
    : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(48, 48, 48, 48);
    m_layout->setSpacing(16);

    auto* code = new QLabel(QLatin1String("ERROR"), this);
    code->setStyleSheet(QStringLiteral(
        "color: %1; font-weight: 800; font-size: 42px; letter-spacing: 6px;")
        .arg(QLatin1String("#ff4d6a")));

    m_errorTitle = new QLabel(this);
    m_errorTitle->setWordWrap(true);
    m_errorTitle->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700;"));

    m_errorDetail = new QLabel(this);
    m_errorDetail->setWordWrap(true);
    m_errorDetail->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;")
                                     .arg(QLatin1String("#9a9eb0")));

    auto* buttons = new QHBoxLayout();
    m_retryButton = new QPushButton(QLatin1String("Retry"), this);
    m_retryButton->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));
    m_prismButton = new QPushButton(QLatin1String("Open Prism"), this);
    m_prismButton->setObjectName(QLatin1String(Constants::OBJ_BUTTON));
    buttons->addWidget(m_retryButton);
    buttons->addWidget(m_prismButton);
    buttons->addStretch(1);

    m_layout->addStretch(2);
    m_layout->addWidget(code);
    m_layout->addWidget(m_errorTitle);
    m_layout->addWidget(m_errorDetail);
    m_layout->addSpacing(12);
    m_layout->addLayout(buttons);
    m_layout->addStretch(3);

    connect(m_retryButton, &QPushButton::clicked, this, &ErrorPanel::retryRequested);
    connect(m_prismButton, &QPushButton::clicked, this, &ErrorPanel::openPrismRequested);
}

void ErrorPanel::showError(const QString& error, const QString& detail) {
    m_errorTitle->setText(error);
    m_errorDetail->setText(detail.isEmpty() ? QLatin1String(Constants::MSG_NO_PRISM) : detail);
}
