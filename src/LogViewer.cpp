#include "LogViewer.h"

#include "PrismBridge.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

LogViewer::LogViewer(PrismBridge* bridge, QWidget* parent)
    : QWidget(parent), m_bridge(bridge) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel(QLatin1String("INSTANCE LOG"), this);
    title->setStyleSheet(QStringLiteral("font-weight: 800; letter-spacing: 2px; font-size: 12px;"));
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8a8a8a; font-size: 11px;"));

    m_followButton = new QPushButton(QLatin1String("Follow: ON"), this);
    m_followButton->setCheckable(true);
    m_followButton->setChecked(true);
    m_copyButton = new QPushButton(QLatin1String("Copy"), this);
    m_closeButton = new QPushButton(QLatin1String("Close"), this);

    header->addWidget(title);
    header->addWidget(m_statusLabel, 1);
    header->addWidget(m_followButton);
    header->addWidget(m_copyButton);
    header->addWidget(m_closeButton);
    layout->addLayout(header);

    m_view = new QPlainTextEdit(this);
    m_view->setObjectName(QLatin1String(Constants::OBJ_LOG_VIEW));
    m_view->setReadOnly(true);
    m_view->setMaximumBlockCount(Constants::LOG_MAX_BLOCKS);
    m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(m_view, 1);

    connect(m_followButton, &QPushButton::toggled, this, [this](bool checked) {
        m_follow = checked;
        m_followButton->setText(checked ? QLatin1String("Follow: ON") : QLatin1String("Follow: OFF"));
    });
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_view->toPlainText());
    });
    connect(m_closeButton, &QPushButton::clicked, this, &LogViewer::closeRequested);

    // Pause follow when the user scrolls up; resume at bottom.
    connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        const bool atBottom = value >= m_view->verticalScrollBar()->maximum();
        if (!atBottom && m_follow) {
            m_follow = false;
            m_followButton->setChecked(false);
        }
    });

    connect(bridge, &PrismBridge::logLineReady, this, &LogViewer::appendLine);
    connect(bridge, &PrismBridge::logStatus, this, &LogViewer::setStatus);
}

void LogViewer::attachToInstance(const QString& instanceId) {
    m_instanceId = instanceId;
    m_view->clear();
    setStatus(QLatin1String(Constants::MSG_WAITING_LOG));
    if (!m_bridge->tailLog(instanceId)) {
        // tailLog already emitted a status line for the missing-log case.
        return;
    }
}

void LogViewer::detach() {
    m_bridge->stopTailing();
    m_instanceId.clear();
}

void LogViewer::appendLine(const QString& line) {
    m_view->appendPlainText(line);
    if (m_follow) {
        m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
    }
}

void LogViewer::setStatus(const QString& message) {
    m_statusLabel->setText(message);
}
