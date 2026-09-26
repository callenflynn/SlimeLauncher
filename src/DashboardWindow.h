#pragma once

#include "Constants.h"
#include "HeroBackdrop.h"
#include "InstanceCard.h"
#include "LogViewer.h"
#include "PrismBridge.h"
#include "SearchOverlay.h"
#include "SettingsView.h"
#include "ThemeManager.h"

#include <QMainWindow>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QVector>

class QLabel;
class QGridLayout;
class QPushButton;
class ControllerLegend;
class ErrorPanel;
class QResizeEvent;

// Full-screen console dashboard. No sidebars, no persistent search field:
// a hero backdrop cross-fades behind the poster grid, a vector controller
// legend anchors the bottom, and a stacked Settings page + modal Search
// overlay complete the three surfaces. Gamepad shoulders cycle
// Library → Settings; Y opens search; X opens the selected card's options.
class DashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DashboardWindow(PrismBridge* bridge, ThemeManager* themes,
                             QWidget* parent = nullptr);

    void refreshInstances();
    void showError(const QString& error, const QString& detail);

    // Console actions bound to GamepadFilter signals (LB/RB views, X/Y,
    // Start/Menu). Public slots so cross-object wiring stays explicit.
public slots:
    void cycleViewPrev() { cycleView(-1); }
    void cycleViewNext() { cycleView(1); }
    void optionsRequested() { showOptionsForSelected(); }
    void searchRequested() { m_searchOverlay->open(m_filterText); }
    void openPrismRequested() { openPrism(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class View { Library = 0, Settings = 1 };

    void buildUi();
    void rebuildGrid(const QVector<InstanceCardModel>& instances);
    void clearGrid();
    void applyFilter(const QString& text);
    void setSelectedInstance(const InstanceCardModel& info);
    void playSelected();
    void openLogs();
    void openPrism();
    void changeArtworkFor(const InstanceCardModel& info);
    void showOptionsForSelected();
    void updateDetailPanel();
    void updateClock();
    void updateAccountChip();
    void updateHeroBackdrop();
    void moveGridFocus(int key);
    void cycleView(int direction);
    void setView(View view);
    int gridColumns() const;
    InstanceCard* cardForId(const QString& id) const;

    PrismBridge* m_bridge;
    ThemeManager* m_themes = nullptr;
    InstanceCardModel m_selected;
    bool m_hasSelection = false;

    // Top bar
    QLabel* m_title = nullptr;
    QLabel* m_account = nullptr;
    QLabel* m_clock = nullptr;
    QTimer* m_clockTimer = nullptr;

    // Center stack + backdrop
    HeroBackdrop* m_backdrop = nullptr;
    QStackedWidget* m_stack = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_gridHost = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QVector<InstanceCard*> m_cards;
    QVector<InstanceCardModel> m_instances;

    // Console footer
    ControllerLegend* m_legend = nullptr;
    QLabel* m_detailLabel = nullptr;
    QPushButton* m_playButton = nullptr;

    // Overlays / secondary views
    SearchOverlay* m_searchOverlay = nullptr;
    SettingsView* m_settings = nullptr;
    LogViewer* m_logViewer = nullptr;
    ErrorPanel* m_errorPanel = nullptr;
    QString m_filterText;
};
