#include "DashboardWindow.h"
#include "ErrorPanel.h"
#include "ImageProcessor.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

DashboardWindow::DashboardWindow(PrismBridge* bridge, QWidget* parent)
    : QMainWindow(parent), m_bridge(bridge) {
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

    // ---- Top bar (glass) ----------------------------------------------------
    auto* topBar = new QWidget(central);
    topBar->setObjectName(QLatin1String(Constants::OBJ_TOP_BAR));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(22, 14, 22, 14);

    m_title = new QLabel(QString::fromUtf8(Constants::APP_NAME).toUpper(), topBar);
    m_title->setStyleSheet(QStringLiteral(
        "font-weight: 800; font-size: 15px; letter-spacing: 4px; color: %1;")
        .arg(QLatin1String("#00f0ff")));

    m_search = new QLineEdit(topBar);
    m_search->setObjectName(QLatin1String(Constants::OBJ_SEARCH));
    m_search->setPlaceholderText(QLatin1String("Filter games…"));
    m_search->setFixedWidth(300);
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &DashboardWindow::applyFilter);

    m_clock = new QLabel(topBar);
    m_clock->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px; font-weight: 600;")
            .arg(QLatin1String("#9a9eb0")));

    topLayout->addWidget(m_title);
    topLayout->addStretch(1);
    topLayout->addWidget(m_search);
    topLayout->addSpacing(18);
    topLayout->addWidget(m_clock);

    // ---- Middle: SideNav + grid stack ---------------------------------------
    auto* middle = new QWidget(central);
    auto* middleLayout = new QHBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    m_sideNav = new SideNav(m_bridge, middle);
    connect(m_sideNav, &SideNav::refreshRequested, this, &DashboardWindow::refreshInstances);
    connect(m_sideNav, &SideNav::openPrismRequested, this, &DashboardWindow::openPrism);
    connect(m_sideNav, &SideNav::allRequested, this, [this]() {
        m_search->clear();
    });

    m_stack = new QStackedWidget(middle);

    // Grid page
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
        QStringLiteral("color: %1; font-size: 15px;").arg(QLatin1String("#9a9eb0")));

    gridLayout->addWidget(m_scroll, 1);
    gridLayout->addWidget(m_emptyLabel, 1);
    m_stack->addWidget(gridPage);

    // Log page
    m_logViewer = new LogViewer(m_bridge, m_stack);
    connect(m_logViewer, &LogViewer::closeRequested, this, [this]() {
        m_stack->setCurrentIndex(0);
        m_logViewer->detach();
    });
    m_stack->addWidget(m_logViewer);

    // Error page
    m_errorPanel = new ErrorPanel(m_stack);
    connect(m_errorPanel, &ErrorPanel::retryRequested, this, [this]() {
        m_stack->setCurrentIndex(0);
        refreshInstances();
    });
    connect(m_errorPanel, &ErrorPanel::openPrismRequested, this, &DashboardWindow::openPrism);
    m_stack->addWidget(m_errorPanel);

    middleLayout->addWidget(m_sideNav);
    middleLayout->addWidget(m_stack, 1);

    // ---- Bottom control strip (glass) ----------------------------------------
    auto* bottom = new QWidget(central);
    bottom->setObjectName(QLatin1String(Constants::OBJ_BOTTOM_BAR));
    auto* bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(22, 12, 22, 12);

    m_detailLabel = new QLabel(QLatin1String(Constants::MSG_NOTHING_SELECTED), bottom);
    m_detailLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(QLatin1String("#9a9eb0")));

    m_playButton = new QPushButton(QLatin1String("▶  Play"), bottom);
    m_playButton->setObjectName(QLatin1String(Constants::OBJ_PRIMARY_BUTTON));
    m_playButton->setEnabled(false);
    m_logsButton = new QPushButton(QLatin1String("Logs"), bottom);
    m_logsButton->setEnabled(false);
    m_prismButton = new QPushButton(QLatin1String("Open Prism"), bottom);
    m_prismButton->setObjectName(QLatin1String(Constants::OBJ_BUTTON));

    bottomLayout->addWidget(m_detailLabel, 1);
    bottomLayout->addWidget(m_playButton);
    bottomLayout->addWidget(m_logsButton);
    bottomLayout->addWidget(m_prismButton);

    connect(m_playButton, &QPushButton::clicked, this, &DashboardWindow::playSelected);
    connect(m_logsButton, &QPushButton::clicked, this, &DashboardWindow::openLogs);
    connect(m_prismButton, &QPushButton::clicked, this, &DashboardWindow::openPrism);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(middle, 1);
    rootLayout->addWidget(bottom);

    setCentralWidget(central);
}

void DashboardWindow::refreshInstances() {
    QString error;
    m_instances = m_bridge->scanInstances(&error);
    if (!error.isEmpty()) {
        showError(error, QString());
        return;
    }
    m_stack->setCurrentIndex(0);
    rebuildGrid(m_instances);
}

void DashboardWindow::showError(const QString& error, const QString& detail) {
    m_errorPanel->showError(error, detail);
    m_stack->setCurrentIndex(2);
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

    int columns = qBound(Constants::CARD_COLUMNS_MIN,
                         qMax(1, (width() - 2 * Constants::GRID_MARGIN + Constants::GRID_SPACING) /
                                     (Constants::CARD_WIDTH + Constants::GRID_SPACING)),
                         Constants::CARD_COLUMNS_MAX);

    int row = 0;
    int col = 0;
    for (const InstanceCardModel& info : instances) {
        auto* card = new InstanceCard(info, m_gridHost);
        connect(card, &InstanceCard::selected, this, &DashboardWindow::setSelectedInstance);
        connect(card, &InstanceCard::activated, this, [this](const InstanceCardModel& target) {
            setSelectedInstance(target);
            playSelected();
        });
        connect(card, &InstanceCard::editRequested, this, [this](const InstanceCardModel& target) {
            const OpResult result = m_bridge->openPrismUi();
            if (!result.ok) {
                showError(result.error, QString());
            }
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
}

void DashboardWindow::updateDetailPanel() {
    if (!m_hasSelection) {
        m_detailLabel->setText(QLatin1String(Constants::MSG_NOTHING_SELECTED));
        m_playButton->setEnabled(false);
        m_logsButton->setEnabled(false);
        return;
    }
    m_detailLabel->setText(QStringLiteral("%1  •  %2  •  %3").arg(
        m_selected.name, m_selected.loader, m_selected.gameVersion.isEmpty()
                              ? QStringLiteral("version unknown")
                              : QStringLiteral("MC %1").arg(m_selected.gameVersion)));
    m_playButton->setEnabled(true);
    m_logsButton->setEnabled(true);
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
    m_stack->setCurrentIndex(1);
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

void DashboardWindow::updateClock() {
    m_clock->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

void DashboardWindow::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();

    // Grid spatial navigation — shared by arrows, D-pad and left stick
    // (GamepadFilter synthesizes arrow keys for those).
    if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {
        if (m_stack->currentIndex() == 0 && !m_cards.isEmpty() && !m_search->hasFocus()) {
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
            if (m_stack->currentIndex() != 0) {
                m_stack->setCurrentIndex(0);
                m_logViewer->detach();
                event->accept();
                return;
            }
            if (!m_search->text().isEmpty()) {
                m_search->clear();
                event->accept();
                return;
            }
            break;
        case Qt::Key_L:
            openLogs();
            event->accept();
            return;
        case Qt::Key_O:
            openPrism();
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
    const int available = width() - m_sideNav->width() - 2 * Constants::GRID_MARGIN;
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
}
