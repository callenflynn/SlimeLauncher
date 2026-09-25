#include "DashboardWindow.h"
#include "ErrorPanel.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
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

    // ---- Top bar ----------------------------------------------------------
    auto* topBar = new QWidget(central);
    topBar->setStyleSheet(QStringLiteral("background: #0d0d0d; border-bottom: 1px solid #262626;"));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(18, 12, 18, 12);

    m_title = new QLabel(QString::fromUtf8(Constants::APP_NAME).toUpper(), topBar);
    m_title->setStyleSheet(QStringLiteral("font-weight: 900; font-size: 16px; letter-spacing: 3px;"));

    m_search = new QLineEdit(topBar);
    m_search->setPlaceholderText(QLatin1String("Filter instances…"));
    m_search->setFixedWidth(280);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (InstanceCard* card : m_cards) {
            const bool visible = card->info().name.contains(text, Qt::CaseInsensitive);
            card->setVisible(visible);
        }
    });

    m_clock = new QLabel(topBar);
    m_clock->setStyleSheet(QStringLiteral("color: #8a8a8a; font-size: 12px;"));

    topLayout->addWidget(m_title);
    topLayout->addStretch(1);
    topLayout->addWidget(m_search);
    topLayout->addSpacing(18);
    topLayout->addWidget(m_clock);

    // ---- Middle: SideNav + grid stack + log viewer ------------------------
    auto* middle = new QWidget(central);
    auto* middleLayout = new QHBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    m_sideNav = new SideNav(m_bridge, middle);
    connect(m_sideNav, &SideNav::refreshRequested, this, &DashboardWindow::refreshInstances);
    connect(m_sideNav, &SideNav::openPrismRequested, this, &DashboardWindow::openPrism);
    connect(m_sideNav, &SideNav::allRequested, this, [this]() {
        m_search->clear();
        for (InstanceCard* card : m_cards) {
            card->setVisible(true);
        }
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
    m_gridHost = new QWidget(m_scroll);
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setContentsMargins(18, 18, 18, 18);
    m_grid->setSpacing(Constants::CARD_SPACING);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_scroll->setWidget(m_gridHost);

    m_emptyLabel = new QLabel(QLatin1String(Constants::MSG_EMPTY_GRID), gridPage);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #8a8a8a; font-size: 14px;"));

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

    // ---- Bottom control strip ---------------------------------------------
    auto* bottom = new QWidget(central);
    bottom->setStyleSheet(QStringLiteral("background: #0d0d0d; border-top: 1px solid #262626;"));
    auto* bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(18, 12, 18, 12);

    m_detailLabel = new QLabel(QLatin1String(Constants::MSG_NOTHING_SELECTED), bottom);
    m_detailLabel->setStyleSheet(QStringLiteral("color: #8a8a8a; font-size: 12px;"));

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

    // Hero card: most recently played instance gets double height + width.
    InstanceCardModel hero = instances.first();
    for (const InstanceCardModel& info : instances) {
        if (info.lastPlayed > hero.lastPlayed) {
            hero = info;
        }
    }

    int row = 0;
    int col = 0;
    for (const InstanceCardModel& info : instances) {
        auto* card = new InstanceCard(info, m_gridHost);
        const bool isHero = (info.id == hero.id);
        card->setObjectName(QLatin1String(isHero ? Constants::OBJ_HERO_CARD
                                                 : Constants::OBJ_INSTANCE_CARD));
        if (isHero) {
            card->setFixedSize(Constants::CARD_SIZE * 2 + QSize(Constants::CARD_SPACING, Constants::CARD_SPACING));
        }
        connect(card, &InstanceCard::selected, this, &DashboardWindow::setSelectedInstance);
        connect(card, &InstanceCard::activated, this, [this](const InstanceCardModel& info) {
            setSelectedInstance(info);
            playSelected();
        });
        m_grid->addWidget(card, row, col);
        m_cards.append(card);

        if (++col >= Constants::GRID_MAX_COLUMNS) {
            col = 0;
            ++row;
        }
    }

    // Select the hero card by default for immediate gamepad play.
    setSelectedInstance(hero);
}

void DashboardWindow::clearGrid() {
    for (InstanceCard* card : m_cards) {
        m_grid->removeWidget(card);
        card->deleteLater();
    }
    m_cards.clear();
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

void DashboardWindow::updateClock() {
    m_clock->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

void DashboardWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
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
