#pragma once

#include <QImage>
#include <QWidget>

class QPaintEvent;

// Full-window ambient backdrop behind the game grid. Holds two composited
// images (current + incoming) and cross-fades between them whenever the
// dashboard points it at a newly selected instance. Fade runs on a 16 ms
// timer; a static vertical charcoal gradient is the fallback surface.
class HeroBackdrop : public QWidget {
    Q_OBJECT

public:
    explicit HeroBackdrop(QWidget* parent = nullptr);

    // Cross-fades to `image` (null resets to the plain gradient).
    void transitionTo(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_current;
    QImage m_next;
    qreal m_progress = 1.0;  // 0..1 blend from m_current to m_next
};
