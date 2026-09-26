#pragma once

#include "Constants.h"
#include "InstanceCardModel.h"

#include <QImage>
#include <QVariantAnimation>
#include <QWidget>

class QPaintEvent;
class QMouseEvent;
class QKeyEvent;
class QContextMenuEvent;

// Poster-style instance card (2:3, Steam Big Picture / Playnite style).
//
// The whole card is painter-drawn: rounded 2:3 poster, dark bottom gradient
// overlay, name + version tag + loader pill, NOW PLAYING badge, neon focus
// ring with glow, and a hover/context action strip. Focus (hover, selection
// or keyboard) animates a smooth 1.0 → FOCUS_SCALE transform.
class InstanceCard : public QWidget {
    Q_OBJECT

public:
    explicit InstanceCard(const InstanceCardModel& info, QWidget* parent = nullptr);
    ~InstanceCard() override;

    void setInfo(const InstanceCardModel& info);
    const InstanceCardModel& info() const { return m_info; }

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

    // Opens the Play / Artwork / Logs / Edit menu at a global position.
    // Used by the card's own context-menu event and by the dashboard when
    // the gamepad X button requests options for the selected card.
    void showContextMenu(const QPoint& globalPos);

    QSize sizeHint() const override { return QSize(Constants::CARD_WIDTH, Constants::CARD_HEIGHT); }

signals:
    void selected(const InstanceCardModel& info);
    void activated(const InstanceCardModel& info);   // play
    void editRequested(const InstanceCardModel& info);
    void artworkChangeRequested(const InstanceCardModel& info);
    void logsRequested(const InstanceCardModel& info);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool event(QEvent* event) override;  // hover tracking for the action strip

private:
    void startAnimation(qreal target);
    qreal scale() const { return m_scale; }
    QRectF cardRect() const;
    void loadArtwork();
    void updateHover(const QPoint& pos);
    int hitAction(const QPoint& localPos) const;  // -1 none, 0 play, 1 artwork, 2 edit, 3 logs

    InstanceCardModel m_info;
    bool m_selected = false;
    bool m_hovered = false;
    int m_pressedAction = -1;
    qreal m_scale = 1.0;
    QVariantAnimation m_scaleAnimation;
    QImage m_artwork;
};
