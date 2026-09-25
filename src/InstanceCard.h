#pragma once

#include "Constants.h"
#include "InstanceCardModel.h"

#include <QFrame>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

class QLabel;
class QVBoxLayout;

// Minimalist high-contrast instance tile.
//
// Shows: name, version flag strip, loader chip, last-played relative time,
// and a NOW PLAYING badge. Selection = 3px neon ring via dynamic property +
// painted 8px glow in paintEvent (QSS has no box-shadow).
class InstanceCard : public QFrame {
    Q_OBJECT

public:
    explicit InstanceCard(const InstanceCardModel& info, QWidget* parent = nullptr);
    ~InstanceCard() override;

    void setInfo(const InstanceCardModel& info);
    const InstanceCardModel& info() const { return m_info; }

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

signals:
    void selected(const InstanceCardModel& info);
    void activated(const InstanceCardModel& info);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi();
    static QString loaderChipColor(const QString& loader);

    InstanceCardModel m_info;
    bool m_selected = false;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_loaderChip = nullptr;
    QLabel* m_flagsLabel = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_playingBadge = nullptr;
    QVBoxLayout* m_layout = nullptr;
};
