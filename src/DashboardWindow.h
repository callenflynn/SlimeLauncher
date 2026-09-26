#pragma once

#include "Constants.h"
#include "InstanceCard.h"
#include "LogViewer.h"
#include "PrismBridge.h"
#include "SideNav.h"

#include <QMainWindow>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QVector>

class QLabel;
class QLineEdit;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class ErrorPanel;
class QResizeEvent;

// Console-style main window: ambient charcoal background, glass top bar
// (title / search / clock), SideNav, poster-card grid with spatial D-pad
// navigation, bottom control strip (Play / Logs / Open Prism), and an
// ErrorPanel stacked over the dashboard for fatal-path recovery.
class DashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DashboardWindow(PrismBridge* bridge, QWidget* parent = nullptr);

    void refreshInstances();
    void showError(const QString& error, const QString& detail);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void rebuildGrid(const QVector<InstanceCardModel>& instances);
    void clearGrid();
    void applyFilter(const QString& text);
    void setSelectedInstance(const InstanceCardModel& info);
    void playSelected();
    void openLogs();
    void openPrism();
    void changeArtworkFor(const InstanceCardModel& info);
    void updateDetailPanel();
    void updateClock();
    void moveGridFocus(int key);
    int gridColumns() const;
    InstanceCard* cardForId(const QString& id) const;

    PrismBridge* m_bridge;
    InstanceCardModel m_selected;
    bool m_hasSelection = false;

    // Top bar
    QLabel* m_title = nullptr;
    QLineEdit* m_search = nullptr;
    QLabel* m_clock = nullptr;
    QTimer* m_clockTimer = nullptr;

    // Center
    SideNav* m_sideNav = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_gridHost = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QVector<InstanceCard*> m_cards;
    QVector<InstanceCardModel> m_instances;

    // Bottom strip
    QLabel* m_detailLabel = nullptr;
    QPushButton* m_playButton = nullptr;
    QPushButton* m_logsButton = nullptr;
    QPushButton* m_prismButton = nullptr;

    // Log + error surfaces
    LogViewer* m_logViewer = nullptr;
    ErrorPanel* m_errorPanel = nullptr;
    QStackedWidget* m_stack = nullptr;
};
