#include "SearchOverlay.h"

#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

SearchOverlay::SearchOverlay(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("SearchOverlay"));
    setAttribute(Qt::WA_StyledBackground, false);

    auto* layout = new QVBoxLayout(this);
    layout->addStretch(2);

    m_field = new QLineEdit(this);
    m_field->setObjectName(QLatin1String(Constants::OBJ_SEARCH));
    m_field->setPlaceholderText(QStringLiteral("Filter games..."));
    m_field->setFixedWidth(420);
    m_field->setAlignment(Qt::AlignCenter);
    m_field->installEventFilter(this);
    connect(m_field, &QLineEdit::textChanged, this, &SearchOverlay::filterChanged);

    auto* hint = new QLabel(QStringLiteral("ENTER apply  ·  ESC cancel"), this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(QStringLiteral("color: #9a9eb0; font-size: 11px; letter-spacing: 2px;"));

    layout->addWidget(m_field, 0, Qt::AlignHCenter);
    layout->addSpacing(10);
    layout->addWidget(hint, 0, Qt::AlignHCenter);
    layout->addStretch(3);

    hide();
}

void SearchOverlay::open(const QString& initialText) {
    m_visible = true;
    show();
    raise();
    m_field->setText(initialText);
    m_field->setFocus(Qt::PopupFocusReason);
    m_field->selectAll();
}

void SearchOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_visible = false;
        hide();
        emit filterChanged(QString());
        emit closed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool SearchOverlay::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_field && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            m_visible = false;
            hide();
            emit closed();
            key->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SearchOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(10, 10, 14, 215));
}
