#include "InstanceCard.h"

#include "ImageProcessor.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEnterEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {

constexpr int ACTION_COUNT = 4;
// Action strip hitboxes live inside the bottom overlay, computed to match
// paintActionStrip(): four equal 34px-high rows is too tall, so the strip is
// a horizontal band of four 34px-wide glyphs starting after the info block.
constexpr int ACTION_W = 34;
constexpr int ACTION_H = 30;

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

QRectF actionStripRect(const QRectF& card) {
    // Horizontal band anchored bottom-right, above the info block.
    return QRectF(card.right() - (ACTION_W * ACTION_COUNT + 3 * 6) - 12,
                  card.bottom() - 46, ACTION_W * ACTION_COUNT + 3 * 6, ACTION_H);
}

}  // namespace

InstanceCard::InstanceCard(const InstanceCardModel& info, QWidget* parent)
    : QWidget(parent), m_info(info) {
    setFixedSize(Constants::CARD_WIDTH, Constants::CARD_HEIGHT);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    m_scaleAnimation.setStartValue(1.0);
    m_scaleAnimation.setEndValue(1.0);
    m_scaleAnimation.setDuration(Constants::FOCUS_ANIMATION_MS);
    m_scaleAnimation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_scaleAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_scale = value.toReal();
        update();
    });

    loadArtwork();
    setInfo(m_info);
}

InstanceCard::~InstanceCard() = default;

void InstanceCard::loadArtwork() {
    // Slime-owned 2:3 poster first; Prism's own instance icon is the fallback
    // (center-cropped to 2:3 in memory so the grid stays uniform); the
    // deterministic default poster is the last resort.
    m_artwork = QImage();
    if (!m_info.cardPath.isEmpty()) {
        m_artwork = QImage(m_info.cardPath);
    }
    if (m_artwork.isNull() && !m_info.iconPath.isEmpty()) {
        const QImage icon(m_info.iconPath);
        if (!icon.isNull()) {
            m_artwork = ImageProcessor::processToCardRatio(icon);
        }
    }
    if (m_artwork.isNull()) {
        const int index = ImageProcessor::defaultCardIndex(m_info.id);
        if (index >= 0 && index < Constants::DEFAULT_CARD_COUNT) {
            m_artwork = ImageProcessor::processFileToCardRatio(
                QLatin1String(Constants::DEFAULT_CARDS[index]));
        }
    }
}

void InstanceCard::setInfo(const InstanceCardModel& info) {
    const bool artworkPathChanged = m_info.cardPath != info.cardPath;
    m_info = info;
    if (artworkPathChanged || m_artwork.isNull()) {
        loadArtwork();
    }
    update();
}

void InstanceCard::setSelected(bool selected) {
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
}

QRectF InstanceCard::cardRect() const {
    const qreal w = qreal(width()) * m_scale;
    const qreal h = qreal(height()) * m_scale;
    return QRectF((width() - w) / 2.0, (height() - h) / 2.0, w, h);
}

void InstanceCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF card = cardRect();
    const qreal radius = Constants::CARD_RADIUS * m_scale;

    const bool focused = m_selected || m_hovered || hasFocus();

    // Ambient focus glow — a soft radial halo behind the active card.
    if (focused) {
        QPainter::RenderHints oldHints = painter.renderHints();
        painter.setRenderHint(QPainter::Antialiasing, false);
        const QColor glow = Constants::COLOR_ACCENT;
        QRadialGradient halo(card.center(), card.width() * 0.85);
        halo.setColorAt(0.0, QColor(glow.red(), glow.green(), glow.blue(), 60));
        halo.setColorAt(1.0, QColor(glow.red(), glow.green(), glow.blue(), 0));
        painter.fillRect(rect(), halo);
        painter.setRenderHints(oldHints);
    }

    // Poster artwork, clipped to the rounded card.
    QPainterPath clip;
    clip.addRoundedRect(card, radius, radius);
    painter.save();
    painter.setClipPath(clip);

    if (!m_artwork.isNull()) {
        painter.drawImage(card, m_artwork);
    } else {
        // No artwork anywhere: painted charcoal fallback so the card never
        // renders empty.
        QLinearGradient fallback(card.topLeft(), card.bottomLeft());
        fallback.setColorAt(0.0, Constants::COLOR_SURFACE_HI);
        fallback.setColorAt(1.0, Constants::COLOR_BG_DEEP);
        painter.fillPath(clip, fallback);
        painter.setPen(Constants::COLOR_MUTED);
        painter.drawText(card.adjusted(16, 16, -16, -16), Qt::AlignCenter,
                         m_info.name.isEmpty() ? QStringLiteral("?") : m_info.name);
    }

    // Glass sheen across the top third.
    QLinearGradient sheen(card.topLeft(), QPointF(card.left(), card.top() + card.height() * 0.35));
    sheen.setColorAt(0.0, QColor(255, 255, 255, 14));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillRect(card, sheen);

    // Bottom gradient overlay for text legibility.
    QLinearGradient shade(card.left(), card.bottom() - card.height() * 0.52, card.left(), card.bottom());
    shade.setColorAt(0.0, QColor(10, 10, 14, 0));
    shade.setColorAt(1.0, QColor(10, 10, 14, 216));
    painter.fillRect(card, shade);

    // ---- Bottom info block -------------------------------------------------
    const qreal inset = 14.0 * m_scale;
    qreal base = card.bottom() - inset;
    const qreal left = card.left() + inset;
    const qreal right = card.right() - inset;

    if (m_info.playing) {
        // NOW PLAYING pill sits above the name row.
        const QString playingText = QStringLiteral("NOW PLAYING");
        base -= 18.0 * m_scale;
        QFont pillFont = painter.font();
        pillFont.setPixelSize(int(9 * m_scale));
        pillFont.setBold(true);
        pillFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        painter.setFont(pillFont);
        const QRectF pillRect(left, base - 16 * m_scale, painter.fontMetrics().horizontalAdvance(playingText) + 16 * m_scale, 14 * m_scale);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(Constants::COLOR_ACCENT.red(), Constants::COLOR_ACCENT.green(),
                                Constants::COLOR_ACCENT.blue(), 34));
        painter.drawRoundedRect(pillRect, 7 * m_scale, 7 * m_scale);
        painter.setPen(Constants::COLOR_ACCENT);
        painter.drawText(pillRect.adjusted(8 * m_scale, 0, -8 * m_scale, -1), Qt::AlignVCenter | Qt::AlignLeft, playingText);
    }

    // Name row (bold, single line, elided).
    QString nameText = m_info.name;
    QFont nameFont = painter.font();
    nameFont.setPixelSize(int(15 * m_scale));
    nameFont.setBold(true);
    painter.setFont(nameFont);
    QFontMetrics nameMetrics(nameFont);
    const QString elidedName = nameMetrics.elidedText(nameText, Qt::ElideRight, int(right - left));
    painter.setPen(Constants::COLOR_TEXT);
    painter.drawText(QPointF(left, base - 8 * m_scale), elidedName);

    // Version tag + last-played row.
    QFont metaFont = painter.font();
    metaFont.setPixelSize(int(10 * m_scale));
    metaFont.setBold(false);
    painter.setFont(metaFont);
    painter.setPen(Constants::COLOR_MUTED);
    QString metaText = m_info.gameVersion.isEmpty() ? QStringLiteral("MC ?") : QStringLiteral("MC %1").arg(m_info.gameVersion);
    metaText += QStringLiteral("  ·  ") + relativeTime(m_info.lastPlayed);
    painter.drawText(QPointF(left, base + 6 * m_scale), metaText);

    // Loader pill on the right edge of the info block.
    const QString loader = m_info.loader.toUpper();
    QFont chipFont = metaFont;
    chipFont.setPixelSize(int(9 * m_scale));
    chipFont.setBold(true);
    chipFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    painter.setFont(chipFont);
    const qreal chipW = painter.fontMetrics().horizontalAdvance(loader) + 14 * m_scale;
    const QRectF chipRect(right - chipW, base - 2 * m_scale, chipW, 15 * m_scale);
    QColor chipColor = Constants::COLOR_ACCENT2;
    if (loader == QLatin1String("FABRIC")) {
        chipColor = QColor(0xc9, 0xb4, 0x7a);
    } else if (loader == QLatin1String("FORGE")) {
        chipColor = QColor(0x7a, 0x9c, 0xff);
    } else if (loader == QLatin1String("NEOFORGE")) {
        chipColor = QColor(0xff, 0xa5, 0x7a);
    } else if (loader == QLatin1String("QUILT")) {
        chipColor = QColor(0x9d, 0xc9, 0x8a);
    }
    painter.setPen(Qt::NoPen);
    QColor chipBg = chipColor;
    chipBg.setAlpha(36);
    painter.setBrush(chipBg);
    painter.drawRoundedRect(chipRect, 7 * m_scale, 7 * m_scale);
    painter.setPen(chipColor);
    painter.drawText(chipRect, Qt::AlignCenter, loader);

    painter.restore();  // rounded clip

    // ---- Focus ring ----------------------------------------------------------
    if (focused) {
        QPen ring(Constants::COLOR_ACCENT, 2.4);
        ring.setCosmetic(true);
        painter.setPen(ring);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(card.adjusted(-1, -1, 1, 1), radius + 1, radius + 1);
        // Violet under-glow along the ring for the neon dual-tone look.
        QPen under(QColor(Constants::COLOR_ACCENT2.red(), Constants::COLOR_ACCENT2.green(),
                          Constants::COLOR_ACCENT2.blue(), 130), 5.0);
        under.setCosmetic(true);
        painter.setPen(under);
        painter.drawRoundedRect(card.adjusted(-1, -1, 1, 1), radius + 1, radius + 1);
        painter.setPen(Constants::COLOR_ACCENT);
        painter.drawRoundedRect(card.adjusted(-1, -1, 1, 1), radius + 1, radius + 1);
    }

    // ---- Quick action strip (hover / focused reveal) -------------------------
    if (m_hovered || (m_selected && hasFocus())) {
        const QRectF strip = actionStripRect(card);
        painter.setPen(Qt::NoPen);
        QColor stripBg(Constants::COLOR_BG_DEEP.red(), Constants::COLOR_BG_DEEP.green(),
                       Constants::COLOR_BG_DEEP.blue(), 200);
        painter.setBrush(stripBg);
        painter.drawRoundedRect(strip, 8, 8);

        // Vector-drawn action icons: triangle (play), frame + landscape
        // (artwork), pencil (edit), lines (logs). Pure QPainter primitives —
        // independent of installed fonts.
        for (int i = 0; i < ACTION_COUNT; ++i) {
            const QRectF cell(strip.left() + i * (ACTION_W + 6), strip.top(), ACTION_W, ACTION_H);
            const bool hot = (i == m_pressedAction);
            painter.setPen(Qt::NoPen);
            if (hot) {
                painter.setBrush(QColor(Constants::COLOR_ACCENT.red(), Constants::COLOR_ACCENT.green(),
                                        Constants::COLOR_ACCENT.blue(), 40));
                painter.drawRoundedRect(cell.adjusted(2, 2, -2, -2), 6, 6);
            }
            const QColor icon = hot ? Constants::COLOR_ACCENT : Constants::COLOR_TEXT;
            const QRectF box = cell.adjusted(9, 7, -9, -7);
            painter.setPen(QPen(icon, 1.6));
            painter.setBrush(Qt::NoBrush);
            switch (i) {
                case 0: {  // Play: filled triangle
                    QPainterPath tri;
                    tri.moveTo(box.left() + 1, box.top() + 1);
                    tri.lineTo(box.left() + 1, box.bottom());
                    tri.lineTo(box.right(), (box.top() + box.bottom()) / 2);
                    tri.closeSubpath();
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(icon);
                    painter.drawPath(tri);
                    painter.setBrush(Qt::NoBrush);
                    break;
                }
                case 1: {  // Artwork: frame with mountain/sun motif
                    painter.drawRect(box);
                    painter.drawEllipse(QRectF(box.right() - 5, box.top() + 1, 4, 4));
                    QPainterPath ridge;
                    ridge.moveTo(box.left(), box.bottom() - 1);
                    ridge.lineTo(box.left() + box.width() * 0.42, box.top() + box.height() * 0.45);
                    ridge.lineTo(box.right(), box.bottom() - 1);
                    painter.drawPath(ridge);
                    break;
                }
                case 2: {  // Edit: pencil
                    painter.drawLine(box.bottomLeft(), box.topRight());
                    painter.drawLine(box.bottomLeft(), QPointF(box.bottomLeft().x() + 3, box.bottomLeft().y()));
                    painter.drawLine(box.bottomLeft(), QPointF(box.bottomLeft().x(), box.bottomLeft().y() - 3));
                    painter.drawLine(box.topRight(), QPointF(box.topRight().x() - 3, box.topRight().y()));
                    painter.drawLine(box.topRight(), QPointF(box.topRight().x(), box.topRight().y() + 3));
                    break;
                }
                case 3: {  // Logs: three stacked lines
                    for (int line = 0; line < 3; ++line) {
                        const qreal ly = box.top() + line * box.height() / 2.0;
                        painter.drawLine(QPointF(box.left(), ly), QPointF(box.right(), ly));
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void InstanceCard::startAnimation(qreal target) {
    m_scaleAnimation.stop();
    m_scaleAnimation.setStartValue(m_scale);
    m_scaleAnimation.setEndValue(target);
    m_scaleAnimation.start();
}

void InstanceCard::enterEvent(QEnterEvent* event) {
    m_hovered = true;
    startAnimation(Constants::FOCUS_SCALE);
    QWidget::enterEvent(event);
}

void InstanceCard::leaveEvent(QEvent* event) {
    m_hovered = false;
    m_pressedAction = -1;
    startAnimation(1.0);
    QWidget::leaveEvent(event);
}

bool InstanceCard::event(QEvent* event) {
    if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter) {
        updateHover(static_cast<QHoverEvent*>(event)->position().toPoint());
    }
    return QWidget::event(event);
}

void InstanceCard::updateHover(const QPoint& pos) {
    Q_UNUSED(pos);
    // Cursor feedback: pointer hand over action cells, arrow elsewhere.
    const QRectF strip = actionStripRect(cardRect());
    const QPoint local = mapFromGlobal(QCursor::pos());
    setCursor(strip.contains(local) ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void InstanceCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const int action = hitAction(event->pos());
        if (action >= 0) {
            m_pressedAction = action;
            update();
            event->accept();
            return;
        }
        emit selected(m_info);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void InstanceCard::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const int action = hitAction(event->pos());
        const int pressed = m_pressedAction;
        m_pressedAction = -1;
        update();
        if (action >= 0 && action == pressed) {
            switch (action) {
                case 0:
                    emit activated(m_info);
                    break;
                case 1:
                    emit artworkChangeRequested(m_info);
                    break;
                case 2:
                    emit editRequested(m_info);
                    break;
                case 3:
                    emit logsRequested(m_info);
                    break;
                default:
                    break;
            }
            event->accept();
            return;
        }
    }
    QWidget::mouseReleaseEvent(event);
}

int InstanceCard::hitAction(const QPoint& localPos) const {
    const QRectF strip = actionStripRect(cardRect());
    if (!strip.contains(localPos)) {
        return -1;
    }
    const int index = int((localPos.x() - strip.left()) / (ACTION_W + 6));
    return (index >= 0 && index < ACTION_COUNT) ? index : -1;
}

void InstanceCard::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit activated(m_info);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void InstanceCard::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_info);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Menu) {
        showContextMenu(QCursor::pos());
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void InstanceCard::showContextMenu(const QPoint& globalPos) {
    // Public entry point: also invoked by the dashboard when the gamepad X
    // button requests the options menu for the selected card.
    QMenu menu(this);
    QAction* play = menu.addAction(QLatin1String(Constants::MSG_CONTEXT_PLAY));
    menu.addSeparator();
    QAction* artwork = menu.addAction(QLatin1String(Constants::MSG_CONTEXT_ARTWORK));
    QAction* logs = menu.addAction(QLatin1String(Constants::MSG_CONTEXT_LOGS));
    QAction* edit = menu.addAction(QLatin1String(Constants::MSG_CONTEXT_EDIT));

    QAction* chosen = menu.exec(globalPos);
    if (chosen == play) {
        emit activated(m_info);
    } else if (chosen == artwork) {
        emit artworkChangeRequested(m_info);
    } else if (chosen == logs) {
        emit logsRequested(m_info);
    } else if (chosen == edit) {
        emit editRequested(m_info);
    }
}

void InstanceCard::contextMenuEvent(QContextMenuEvent* event) {
    // Selecting on right-click matches console grid behavior.
    emit selected(m_info);
    showContextMenu(event->globalPos());
    event->accept();
}
