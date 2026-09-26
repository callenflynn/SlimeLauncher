#pragma once

#include "Constants.h"

#include <QLineEdit>
#include <QWidget>

// Modal search field in the style of a console UI: a translucent full-screen
// layer with one centered rounded input. Esc closes it; Enter/accept or
// clearing signals are consumed by the dashboard to filter the grid.
class SearchOverlay : public QWidget {
    Q_OBJECT

public:
    explicit SearchOverlay(QWidget* parent = nullptr);

    void open(const QString& initialText);
    bool isSearching() const { return m_visible; }

signals:
    void filterChanged(const QString& text);
    void closed();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QLineEdit* m_field = nullptr;
    bool m_visible = false;
};
