#include "ControllerLegend.h"

#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>

namespace {

constexpr int BADGE_SIZE = 22;   // round badge diameter
constexpr int BADGE_H = 20;      // pill badge height
constexpr int GAP = 6;           // badge-to-label gap
constexpr int ENTRY_GAP = 26;    // gap between legend entries

QColor badgeColor(const QString& text) {
    if (text == QLatin1String("A")) {
        return QColor(0x3f, 0xd9, 0x6c);  // green
    }
    if (text == QLatin1String("X")) {
        return QColor(0x4d, 0x8f, 0xff);  // blue
    }
    if (text == QLatin1String("Y")) {
        return QColor(0xf2, 0xc1, 0x4e);  // yellow
    }
    return QColor(0x9a, 0x9e, 0xb0);  // neutral for LB/RB + MENU
}

}  // namespace

ControllerLegend::ControllerLegend(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
}

void ControllerLegend::drawBadge(QPainter& painter, const QRectF& box, const QString& text,
                                 bool pill) const {
    const QColor color = badgeColor(text);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 38));
    painter.setPen(QPen(color, 1.4));
    if (pill) {
        painter.drawRoundedRect(box, box.height() / 2.0, box.height() / 2.0);
    } else {
        painter.drawEllipse(box);
    }

    QFont font = painter.font();
    font.setPixelSize(text.length() > 2 ? 8 : 11);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(box, Qt::AlignCenter, text);
}

void ControllerLegend::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont labelFont = painter.font();
    labelFont.setPixelSize(12);
    painter.setFont(labelFont);
    const QFontMetrics metrics(labelFont);

    // Measure total width to center the whole legend.
    int total = 0;
    QVector<int> badgeWidths;
    for (const Entry& entry : kEntries) {
        const QString badge = QString::fromLatin1(entry.badge);
        const bool pill = badge.length() > 1;
        const int bw = pill ? metrics.horizontalAdvance(badge) + 14 : BADGE_SIZE;
        const int labelW = metrics.horizontalAdvance(QString::fromLatin1(entry.label));
        badgeWidths.append(bw);
        total += bw + GAP + labelW;
    }
    total += ENTRY_GAP * (int(std::size(kEntries)) - 1);

    int x = (width() - total) / 2;
    const qreal y = (height() - BADGE_H) / 2.0;

    for (int i = 0; i < int(std::size(kEntries)); ++i) {
        const Entry& entry = kEntries[i];
        const QString badge = QString::fromLatin1(entry.badge);
        const bool pill = badge.length() > 1;
        const int bw = badgeWidths[i];
        const QRectF badgeBox(x, y + (BADGE_H - (pill ? BADGE_H : BADGE_SIZE)) / 2.0, bw,
                              pill ? BADGE_H : BADGE_SIZE);
        drawBadge(painter, badgeBox, badge, pill);
        painter.setPen(QColor(0x9a, 0x9e, 0xb0));
        painter.drawText(QPointF(badgeBox.right() + GAP,
                                 y + BADGE_H / 2.0 + metrics.ascent() / 2.0 - 1),
                         QString::fromLatin1(entry.label));
        x += bw + GAP + metrics.horizontalAdvance(QString::fromLatin1(entry.label)) + ENTRY_GAP;
    }
}
