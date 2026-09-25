#include "InstanceCard.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QVBoxLayout>

#include <QDateTime>

namespace {

QString relativeTime(const QDateTime& when) {
    if (!when.isValid()) {
        return QLatin1String("never played");
    }
    const qint64 secs = when.secsTo(QDateTime::currentDateTime());
    if (secs < 0) {
        return QLatin1String("just now");
    }
    if (secs < 3600) {
        return QStringLiteral("%1m ago").arg(secs / 60);
    }
    if (secs < 86400) {
        return QStringLiteral("%1h ago").arg(secs / 3600);
    }
    return QStringLiteral("%1d ago").arg(secs / 86400);
}

}  // namespace

InstanceCard::InstanceCard(const InstanceCardModel& info, QWidget* parent)
    : QFrame(parent), m_info(info) {
    setObjectName(QLatin1String(Constants::OBJ_INSTANCE_CARD));
    setFixedSize(Constants::CARD_SIZE);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    buildUi();
    setInfo(m_info);
}

InstanceCard::~InstanceCard() = default;

void InstanceCard::buildUi() {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(12, 10, 12, 10);
    m_layout->setSpacing(6);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700;"));

    m_loaderChip = new QLabel(this);
    m_loaderChip->setObjectName(QLatin1String(Constants::OBJ_STATUS_CHIP));
    m_loaderChip->setAlignment(Qt::AlignLeft);

    m_flagsLabel = new QLabel(this);
    m_flagsLabel->setStyleSheet(QStringLiteral("font-size: 11px;"));

    m_timeLabel = new QLabel(this);
    m_timeLabel->setStyleSheet(QStringLiteral("font-size: 11px;"));

    m_playingBadge = new QLabel(this);
    m_playingBadge->setStyleSheet(
        QStringLiteral("color: #39ff14; font-weight: 800; font-size: 11px; letter-spacing: 1px;"));
    m_playingBadge->hide();

    m_layout->addWidget(m_nameLabel);
    m_layout->addWidget(m_loaderChip);
    m_layout->addWidget(m_flagsLabel);
    m_layout->addWidget(m_timeLabel);
    m_layout->addStretch(1);
    m_layout->addWidget(m_playingBadge);
}

void InstanceCard::setInfo(const InstanceCardModel& info) {
    m_info = info;
    m_nameLabel->setText(info.name);
    m_loaderChip->setText(info.loader.isEmpty() ? QLatin1String("Unknown") : info.loader.toUpper());
    m_flagsLabel->setText(info.gameVersion.isEmpty() ? QStringLiteral("version unknown")
                                                     : QStringLiteral("MC %1").arg(info.gameVersion));
    m_timeLabel->setText(relativeTime(info.lastPlayed));
    m_playingBadge->setVisible(info.playing);
    m_playingBadge->setText(QLatin1String("NOW PLAYING"));
    setProperty("playing", info.playing);
    style()->unpolish(this);
    style()->polish(this);
}

void InstanceCard::setSelected(bool selected) {
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void InstanceCard::paintEvent(QPaintEvent* event) {
    // Painted glow behind the neon focus ring (QSS has no box-shadow).
    if (m_selected || m_info.playing) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor glow = m_selected ? QColor(0x39, 0xff, 0x14) : QColor(0x39, 0xff, 0x14, 90);
        for (int i = 8; i >= 1; --i) {
            glow.setAlpha(m_selected ? 26 : 10);
            painter.setPen(QPen(glow, 1));
            painter.drawRect(rect().adjusted(-i, -i, i, i));
        }
    }
    QFrame::paintEvent(event);
}

void InstanceCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit selected(m_info);
    }
    QFrame::mousePressEvent(event);
}

void InstanceCard::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit activated(m_info);
    }
    QFrame::mouseDoubleClickEvent(event);
}

void InstanceCard::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_info);
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}
