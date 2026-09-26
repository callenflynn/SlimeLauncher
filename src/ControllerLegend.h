#pragma once

#include <QWidget>

// Console footer: a single controller legend rendered with vector-drawn
// button badges — [A] Play  [X] Options  [Y] Search  [LB/RB] Views
// [Menu] Open Prism. All badges are QPainter primitives (circle / rounded
// pill shapes with a letter inside), so no glyph depends on system fonts.
class ControllerLegend : public QWidget {
    Q_OBJECT

public:
    explicit ControllerLegend(QWidget* parent = nullptr);

    QSize sizeHint() const override { return QSize(640, 44); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Entry {
        const char* badge;  // "A", "X", "Y", "LB/RB", "MENU"
        const char* label;  // ASCII description
    };
    static constexpr Entry kEntries[] = {{"A", "Play"},  {"X", "Options"},  {"Y", "Search"},
                                         {"LB/RB", "Views"}, {"MENU", "Open Prism"}};

    void drawBadge(QPainter& painter, const QRectF& box, const QString& text, bool pill) const;
};
