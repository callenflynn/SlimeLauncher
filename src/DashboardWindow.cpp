#include "DashboardWindow.h"
#include "ControllerLegend.h"
#include "ErrorPanel.h"
#include "ImageProcessor.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

DashboardWindow::DashboardWindow(PrismBridge* bridge, ThemeManager* themes, QWidget* parent)
    : QMainWindow(parent), m_bridge(bridge), m_themes(themes) {
    setWindowTitle(QLatin1String(Constants::APP_NAME));
    resize(Constants::DEFAULT_WIN_SIZE);
    setMinimumSize(Constants::MIN_WIN_SIZE);

    buildUi();

    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(Constants::CLOCK_TICK_MS);
    connect(m_clockTimer, &QTimer::timeout, this, &DashboardWindow::updateClock);
    m_clockTimer->start();
    updateClock();

    refreshInstances();
}

void DashboardWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- Top bar: wordmark · account · clock ---------------------------------
    auto* topBar = new QWidget(central);
    topBar->setObjectName(QLatin1String(Constants::OBJ_TOP_BAR));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(24, 14, 24, 14);

    m_title = new QLabel(QString::fromUtf8(Constants::APP_NAME).toUpper(), topBar);
    m_title->setStyleSheet(QStringLiteral(
        "font-weight: 800; font-size: 15px; letter-spacing: 4px; color: #00f0ff;"));

    m_account = new QLabel(topBar);
    m_account->setAlignment(Qt::AlignCenter);
    m_account->setObjectName(QLatin1String(Constants::OBJ_STATUS_CHIP));

    m_clock = new QLabel(topBar);
    m_clock->setStyleSheet(
        QStringLiteral("color: #9a9eb0; font-size: 12px; font-weight: 600;"));

    topLayout->addWidget(m_title);
    topLayout->addStretch(1);
    topLayout->addWidget(m_account);
    topLayout->addStretch(1);
    topLayout->addWidget(m_clock);

    // ---- Content: hero backdrop under a stacked center ------------------------
    auto* center = new QWidget(central);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_backdrop = new HeroBackdrop(center);

    m_stack = new QStackedWidget(center);
    m_stack->setAttribute(Qt::WA_StyledBackground, false);
    m_stack->setStyleSheet(QStringLiteral("background: transparent;"));

    // Library page: the poster grid over the backdrop.
    auto* gridPage = new QWidget(m_stack);
    auto* gridLayout = new QVBoxLayout(gridPage);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);

    m_scroll = new QScrollArea(gridPage);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_gridHost = new QWidget(m_scroll);
    m_gridHost->setStyleSheet(QStringLiteral("background: transparent;"));
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setContentsMargins(Constants::GRID_MARGIN, Constants::GRID_MARGIN, Constants::GRID_MARGIN,
                               Constants::GRID_MARGIN);
    m_grid->setSpacing(Constants::GRID_SPACING);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_scroll->setWidget(m_gridHost);

    m_emptyLabel = new QLabel(QLatin1String(Constants::MSG_EMPTY_GRID), gridPage);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(
        QStringLiteral("color: #9a9eb0; font-size: 15px;"));

    gridLayout->addWidget(m_scroll, 1);
    gridLayout->addWidget(m_emptyLabel, 1);
    m_stack->addWidget(gridPage);   // index 0 — Library

    // Settings page
    m_settings = new SettingsView(m_bridge, m_themes, m_stack);
    connect(m_settings, &SettingsView::openPrismRequested, this, &DashboardWindow::openPrism);
    connect(m_settings, &SettingsView::refreshRequested, this, &DashboardWindow::refreshInstances);
    connect(m_settings, &SettingsView::themeToggled, this, [this](ThemeManager::Theme theme) {
        // Persist only once paths are known (wizard owns first-run writes).
        if (m_themes) {
            m_themes->persistTheme(theme);
        }
    });
    m_stack->addWidget(m_settings);  // index 1 — Settings

    // Log page
    m_logViewer = new LogViewer(m_bridge, m_stack);
    connect(m_logViewer, &LogViewer::closeRequested, this, [this]() {
        m_stack->setCurrentIndex(0);
        m_logViewer->detach();
    });
    m_stack->addWidget(m_logViewer);  // index 2 — Logs

    // Error page
    m_errorPanel = new ErrorPanel(m_stack);
    connect(m_errorPanel, &ErrorPanel::retryRequested, this, [this]() {
        m_stack->setCurrentIndex(0);
        refreshInstances();
    });
    connect(m_errorPanel, &ErrorPanel::openPrismRequested, this, &DashboardWindow::openPrism);
    m_stack->addWidget(m_errorPanel);  // index 3 — Error

    centerLayout->addWidget(m_stack, 1);

    // ---- Console footer: detail line + controller legend ----------------------
    auto* bottom = new QWidget(central);
    bottom->setObjectName(QLatin1String(Constants::OBJ_BOTTOM_BAR));
    auto* bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(24, 10, 24, 10);

    m_detailLabel = new QLabel(QLatin1String(Constants::MSG_NOTHING_SELECTED), bottom);
    m_detailLabel->setStyleSheet(
        QStringLiteral("color: #9a9eb0; font-size: 12px;"));

    m_playButton = new QPushButton(QLatin1String(Constants::MSG_CONTEXT_PLAY), bottom);
    m_playButton->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));
    m_playButton->setEnabled(false);

    m_legend = new ControllerLegend(bottom);

    bottomLayout->addWidget(m_detailLabel, 1);
    bottomLayout->addWidget(m_playButton);
    bottomLayout->addSpacing(16);
    bottomLayout->addWidget(m_legend);

    connect(m_playButton, &QPushButton::clicked, this, &DashboardWindow::playSelected);

    // ---- Search overlay (modal, on demand) ------------------------------------
    m_searchOverlay = new SearchOverlay(central);
    connect(m_searchOverlay, &SearchOverlay::filterChanged, this, [this](const QString& text) {
        m_filterText = text;
        applyFilter(text);
    });

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(center, 1);
    rootLayout->addWidget(bottom);

    setCentralWidget(central);
    updateAccountChip();
}

void DashboardWindow::refreshInstances() {
    QString error;
    m_instances = m_bridge->scanInstances(&error);
    if (!error.isEmpty()) {
        showError(error, QString());
        return;
    }
    if (m_stack->currentIndex() == 3) {
        m_stack->setCurrentIndex(0);
    }
    rebuildGrid(m_instances);
    updateAccountChip();
}

void DashboardWindow::showError(const QString& error, const QString& detail) {
    m_errorPanel->showError(error, detail);
    m_stack->setCurrentIndex(3);
}

void DashboardWindow::rebuildGrid(const QVector<InstanceCardModel>& instances) {
    clearGrid();

    const bool empty = instances.isEmpty();
    m_emptyLabel->setVisible(empty);
    m_scroll->setVisible(!empty);
    if (empty) {
        m_hasSelection = false;
        updateDetailPanel();
        return;
    }

    const int columns = gridColumns();
    int row = 0;
    int col = 0;
    for (const InstanceCardModel& info : instances) {
        auto* card = new InstanceCard(info, m_gridHost);
        connect(card, &InstanceCard::selected, this, &DashboardWindow::setSelectedInstance);
        connect(card, &InstanceCard::activated, this, [this](const InstanceCardModel& target) {
            setSelectedInstance(target);
            playSelected();
        });
        connect(card, &InstanceCard::editRequested, this, [this](const InstanceCardModel&) {
            openPrism();
        });
        connect(card, &InstanceCard::artworkChangeRequested, this,
                &DashboardWindow::changeArtworkFor);
        connect(card, &InstanceCard::logsRequested, this, [this](const InstanceCardModel& target) {
            setSelectedInstance(target);
            openLogs();
        });
        m_grid->addWidget(card, row, col);
        m_cards.append(card);

        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    // Re-apply the active filter after a rescan.
    if (!m_filterText.isEmpty()) {
        applyFilter(m_filterText);
    }

    // Select the most recently played card by default for instant gamepad play.
    InstanceCardModel hero = instances.first();
    for (const InstanceCardModel& info : instances) {
        if (info.lastPlayed > hero.lastPlayed) {
            hero = info;
        }
    }
    setSelectedInstance(hero);
    if (InstanceCard* heroCard = cardForId(hero.id)) {
        heroCard->setFocus();
    }
}

InstanceCard* DashboardWindow::cardForId(const QString& id) const {
    for (InstanceCard* card : m_cards) {
        if (card->info().id == id) {
            return card;
        }
    }
    return nullptr;
}

void DashboardWindow::clearGrid() {
    for (InstanceCard* card : m_cards) {
        m_grid->removeWidget(card);
        card->deleteLater();
    }
    m_cards.clear();
}

void DashboardWindow::applyFilter(const QString& text) {
    for (InstanceCard* card : m_cards) {
        card->setVisible(card->info().name.contains(text, Qt::CaseInsensitive));
    }
}

void DashboardWindow::setSelectedInstance(const InstanceCardModel& info) {
    m_selected = info;
    m_hasSelection = true;
    for (InstanceCard* card : m_cards) {
        card->setSelected(card->info().id == info.id);
    }
    updateDetailPanel();
    updateHeroBackdrop();
}

void DashboardWindow::updateHeroBackdrop() {
    if (!m_backdrop) {
        return;
    }
    const QImage backdrop = m_hasSelection
        ? ImageProcessor::heroBackdropImage(m_bridge->instancesDir(), m_selected.id, size())
        : QImage();
    m_backdrop->transitionTo(backdrop);
}

void DashboardWindow::updateDetailPanel() {
    if (!m_hasSelection) {
        m_detailLabel->setText(QLatin1String(Constants::MSG_NOTHING_SELECTED));
        m_playButton->setEnabled(false);
        return;
    }
    m_detailLabel->setText(QStringLiteral("%1  ·  %2  ·  %3").arg(
        m_selected.name, m_selected.loader, m_selected.gameVersion.isEmpty()
                              ? QStringLiteral("version unknown")
                              : QStringLiteral("MC %1").arg(m_selected.gameVersion)));
    m_playButton->setEnabled(true);
}

void DashboardWindow::updateAccountChip() {
    const QVector<AccountInfo> accounts = m_bridge->readAccounts();
    if (accounts.isEmpty()) {
        m_account->setText(QLatin1String(Constants::MSG_ACCOUNT_NONE));
        return;
    }
    const AccountInfo& active = accounts.first();
    m_account->setText(active.active
                           ? QString::fromUtf8(Constants::MSG_ACCOUNT_IN).arg(active.name)
                           : QString::fromUtf8(Constants::MSG_ACCOUNT_OFF).arg(active.name));
}

void DashboardWindow::playSelected() {
    if (!m_hasSelection) {
        return;
    }
    const OpResult result = m_bridge->launchInstance(m_selected.id);
    if (!result.ok) {
        showError(result.error, QString());
        return;
    }
    m_selected.playing = true;
    updateDetailPanel();
    refreshInstances();
}

void DashboardWindow::openLogs() {
    if (!m_hasSelection) {
        return;
    }
    m_logViewer->attachToInstance(m_selected.id);
    m_stack->setCurrentIndex(2);
}

void DashboardWindow::openPrism() {
    const OpResult result = m_bridge->openPrismUi();
    if (!result.ok) {
        showError(result.error, QString());
    }
}

void DashboardWindow::changeArtworkFor(const InstanceCardModel& info) {
    const QString source = QFileDialog::getOpenFileName(
        this, QLatin1String(Constants::MSG_ARTWORK_PICK_TITLE), QDir::homePath(),
        QLatin1String(Constants::MSG_ARTWORK_PICK_FILTER));
    if (source.isEmpty()) {
        return;
    }
    const OpResult result = m_bridge->setInstanceCardArtwork(info.id, source);
    if (!result.ok) {
        QMessageBox::warning(this, QLatin1String(Constants::APP_NAME),
                             QLatin1String(Constants::MSG_ARTWORK_FAILED) + QStringLiteral("\n\n") +
                                 result.error);
        return;
    }
    refreshInstances();
}

void DashboardWindow::showOptionsForSelected() {
    if (!m_hasSelection) {
        return;
    }
    if (InstanceCard* card = cardForId(m_selected.id)) {
        card->showContextMenu(QCursor::pos());
    }
}

void DashboardWindow::updateClock() {
    m_clock->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

void DashboardWindow::cycleView(int direction) {
    // Library ↔ Settings; Logs and Error are contextual, not cycled.
    const View current =
        (m_stack->widget(1) == m_stack->currentWidget()) ? View::Settings : View::Library;
    if (direction >= 0 && current == View::Library) {
        setView(View::Settings);
    } else if (direction < 0 && current == View::Settings) {
        setView(View::Library);
    }
}

void DashboardWindow::setView(View view) {
    if (view == View::Settings) {
        m_settings->refreshAccountChip();
        m_stack->setCurrentIndex(1);
    } else {
        m_stack->setCurrentIndex(0);
        if (!m_cards.isEmpty()) {
            InstanceCard* card = cardForId(m_selected.id);
            if (!card && !m_cards.isEmpty()) {
                card = m_cards.first();
            }
            if (card) {
                card->setFocus();
            }
        }
    }
}

void DashboardWindow::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();

    // Grid spatial navigation — shared by arrows, D-pad and left stick
    // (GamepadFilter synthesizes arrow keys for those).
    if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {
        if (m_stack->currentIndex() == 0 && !m_cards.isEmpty() &&
            !m_searchOverlay->isSearching()) {
            moveGridFocus(key);
            event->accept();
            return;
        }
    }

    switch (key) {
        case Qt::Key_F5:
            refreshInstances();
            event->accept();
            return;
        case Qt::Key_Escape:
            if (m_stack->currentIndex() == 2) {
                m_stack->setCurrentIndex(0);
                m_logViewer->detach();
                event->accept();
                return;
            }
            if (m_stack->currentIndex() == 1) {
                setView(View::Library);
                event->accept();
                return;
            }
            if (!m_filterText.isEmpty()) {
                m_filterText.clear();
                applyFilter(m_filterText);
                event->accept();
                return;
            }
            break;
        case Qt::Key_E:  // Options — mirrors gamepad X
            showOptionsForSelected();
            event->accept();
            return;
        case Qt::Key_F:  // Search — mirrors gamepad Y
            m_searchOverlay->open(m_filterText);
            event->accept();
            return;
        case Qt::Key_O:  // Open Prism — mirrors gamepad Start/Menu
            openPrism();
            event->accept();
            return;
        case Qt::Key_L:
            openLogs();
            event->accept();
            return;
        case Qt::Key_BracketLeft:  // LB fallback on keyboard
            cycleView(-1);
            event->accept();
            return;
        case Qt::Key_BracketRight:  // RB fallback on keyboard
            cycleView(1);
            event->accept();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            playSelected();
            event->accept();
            return;
        default:
            break;
    }
    QMainWindow::keyPressEvent(event);
}

void DashboardWindow::moveGridFocus(int key) {
    InstanceCard* current = m_hasSelection ? cardForId(m_selected.id) : nullptr;
    if (!current) {
        if (!m_cards.isEmpty()) {
            m_cards.first()->setFocus();
            setSelectedInstance(m_cards.first()->info());
        }
        return;
    }

    const int columns = gridColumns();
    const int index = static_cast<int>(m_cards.indexOf(current));
    if (index < 0) {
        return;
    }
    int target = index;
    switch (key) {
        case Qt::Key_Left:
            target = (index % columns == 0) ? index : index - 1;
            break;
        case Qt::Key_Right:
            target = (index + 1 >= m_cards.size()) ? index : index + 1;
            break;
        case Qt::Key_Up:
            target = (index - columns >= 0) ? index - columns : index;
            break;
        case Qt::Key_Down:
            target = (index + columns < m_cards.size()) ? index + columns : index;
            break;
        default:
            return;
    }
    if (target == index) {
        return;
    }
    InstanceCard* next = m_cards.at(target);
    setSelectedInstance(next->info());
    next->setFocus();
    m_scroll->ensureWidgetVisible(next, 24, 24);
}

int DashboardWindow::gridColumns() const {
    const int available = width() - 2 * Constants::GRID_MARGIN;
    return qBound(Constants::CARD_COLUMNS_MIN,
                  qMax(1, (available + Constants::GRID_SPACING) /
                              (Constants::CARD_WIDTH + Constants::GRID_SPACING)),
                  Constants::CARD_COLUMNS_MAX);
}

void DashboardWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Re-flow the grid to the new column count; cards keep their own size.
    if (!m_instances.isEmpty()) {
        rebuildGrid(m_instances);
    }
    if (!m_selected.id.isEmpty()) {
        updateHeroBackdrop();
    }
}
