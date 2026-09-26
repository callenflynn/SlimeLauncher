#include "HeroBackdrop.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>

namespace {
constexpr int FADE_TICK_MS = 16;
constexpr int FADE_DURATION_TICKS = 26;  // ~420 ms
}  // namespace

HeroBackdrop::HeroBackdrop(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
}

void HeroBackdrop::transitionTo(const QImage& image) {
    if (m_progress < 1.0 && image.cacheKey() == m_next.cacheKey()) {
        return;  // fade already in flight toward the same image
    }
    if (m_progress >= 1.0 && image.cacheKey() == m_current.cacheKey()) {
        return;  // already showing it
    }
    m_next = image;
    m_progress = 0.0;

    auto* ticker = new QTimer(this);
    connect(ticker, &QTimer::timeout, this, [this, ticker]() {
        m_progress = qMin(1.0, m_progress + 1.0 / qreal(FADE_DURATION_TICKS));
        if (m_progress >= 1.0) {
            m_current = m_next;
            m_next = QImage();
            ticker->stop();
            ticker->deleteLater();
        }
        update();
    });
    ticker->start(FADE_TICK_MS);
}

void HeroBackdrop::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Charcoal vertical wash under everything (also the fade-out target).
    QLinearGradient base(rect().topLeft(), rect().bottomLeft());
    base.setColorAt(0.0, QColor(0x0f, 0x0f, 0x13));
    base.setColorAt(1.0, QColor(0x0a, 0x0a, 0x0e));
    painter.fillRect(rect(), base);

    const auto drawCover = [&painter](const QImage& image, const QRectF& box) {
        if (image.isNull()) {
            return;
        }
        // Cover-fit: scale until both axes are filled, then center-crop.
        QSizeF scaled = QSizeF(image.size()).scaled(box.width(), box.height(), Qt::KeepAspectRatioByExpanding);
        const QRectF target(box.left() + (box.width() - scaled.width()) / 2.0,
                            box.top() + (box.height() - scaled.height()) / 2.0, scaled.width(),
                            scaled.height());
        painter.drawImage(target, image);
    };

    if (m_progress < 1.0 && !m_next.isNull()) {
        drawCover(m_current, rect());
        painter.setOpacity(m_progress);
        drawCover(m_next, rect());
        painter.setOpacity(1.0);
    } else if (!m_current.isNull()) {
        drawCover(m_current, rect());
    }

    // Fixed darkening scrim so card text and the grid always read clearly.
    QLinearGradient scrim(rect().topLeft(), rect().bottomLeft());
    scrim.setColorAt(0.0, QColor(10, 10, 14, 150));
    scrim.setColorAt(0.55, QColor(10, 10, 14, 190));
    scrim.setColorAt(1.0, QColor(10, 10, 14, 225));
    painter.fillRect(rect(), scrim);
}
