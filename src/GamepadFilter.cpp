#include "GamepadFilter.h"

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QWidget>

#include <fcntl.h>
#include <unistd.h>

namespace {

// js_event button indices (Linux joystick API, matches most controllers).
// NOTE: BTN_A/B/X/Y are kernel macros — use k-prefixed names here.
constexpr int kBtnA = 0;
constexpr int kBtnB = 1;
constexpr int kBtnX = 2;
constexpr int kBtnY = 3;
constexpr int kBtnLb = 4;
constexpr int kBtnRb = 5;
constexpr int kBtnBack = 8;
constexpr int kBtnStart = 9;
constexpr int kBtnMenu = 10;

// D-pad axes: many pads report hat-switch style axes 5/6 in -32767..32767.
constexpr int AXIS_DPAD_X = 5;
constexpr int AXIS_DPAD_Y = 6;
constexpr int AXIS_LEFT_X = 0;
constexpr int AXIS_LEFT_Y = 1;

// Console actions dispatched for buttons that have a semantic role beyond
// the shared keyboard mapping. Order mirrors the signal declarations in
// GamepadFilter.h.
enum class PadAction {
    None = 0,
    Play,          // A — activate the focused item
    Back,          // B / Back — escape
    Options,       // X — card options menu
    Search,        // Y — search overlay
    PrevView,      // LB — previous view
    NextView,      // RB — next view
    Prism,         // Start / Menu — open native Prism
};

// js_event type flags
constexpr auto TYPE_BUTTON = JS_EVENT_BUTTON;
constexpr auto TYPE_AXIS = JS_EVENT_AXIS;

qint16 deadzoneThreshold() {
    return static_cast<qint16>((32767 * Constants::GAMEPAD_DEADZONE_PCT) / 100);
}

}  // namespace

GamepadFilter::GamepadFilter(QObject* parent)
    : QObject(parent), QAbstractNativeEventFilter() {
    scanDevices();
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(16);
    connect(m_pollTimer, &QTimer::timeout, this, &GamepadFilter::pump);
    m_pollTimer->start();
}

GamepadFilter::~GamepadFilter() {
    closeAll();
}

void GamepadFilter::scanDevices() {
    QDir inputDir(QStringLiteral("/dev/input"));
    const QStringList jsDevices = inputDir.entryList(QStringList() << QStringLiteral("js*"), QDir::System, QDir::Name);
    int index = 0;
    for (const QString& name : jsDevices) {
        if (m_pads.size() >= 4) {
            break;
        }
        const QString path = inputDir.filePath(name);
        bool alreadyOpen = false;
        for (PadState* pad : m_pads) {
            if (pad->devicePath == path) {
                alreadyOpen = true;
                break;
            }
        }
        if (alreadyOpen) {
            continue;
        }
        const int fd = ::open(path.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }
        auto* pad = new PadState{};
        pad->fd = fd;
        pad->devicePath = path;
        pad->buttonStates.resize(32);
        pad->axisStates.resize(16);
        m_pads.insert(index++, pad);
    }
}

void GamepadFilter::closeAll() {
    for (PadState* pad : m_pads) {
        if (pad->fd >= 0) {
            ::close(pad->fd);
        }
        delete pad;
    }
    m_pads.clear();
}

void GamepadFilter::pump() {
    // Hot-plug: re-scan occasionally so new pads appear without restart.
    static int rescanCounter = 0;
    if (++rescanCounter >= 120) {  // roughly every 2 seconds at 16ms
        rescanCounter = 0;
        scanDevices();
    }

    for (PadState* pad : m_pads) {
        if (pad->fd < 0) {
            continue;
        }
        js_event ev{};
        while (::read(pad->fd, &ev, sizeof(ev)) == static_cast<ssize_t>(sizeof(ev))) {
            handleEvent(*pad, ev);
        }

        // Hold-repeat: after the initial delay, repeat the held key.
        if (pad->heldKey != 0 && pad->repeatTimer.isValid()) {
            const qint64 elapsed = pad->repeatTimer.elapsed();
            const qint64 threshold = pad->repeatFired ? Constants::GAMEPAD_REPEAT_INTERVAL_MS
                                                      : Constants::GAMEPAD_REPEAT_DELAY_MS;
            if (elapsed >= threshold) {
                postKey(pad->heldKey, true);
                pad->repeatTimer.restart();
                pad->repeatFired = true;
            }
        }
    }
}

bool GamepadFilter::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;  // never consume native events; polling does the work
}

bool GamepadFilter::mapButton(int index, int* key) {
    if (!key) {
        return false;
    }
    // Shoulders never post keys — they drive view switching via signals only.
    switch (index) {
        case kBtnA: *key = Qt::Key_Return; return true;
        case kBtnB: *key = Qt::Key_Escape; return true;
        case kBtnX: *key = Qt::Key_E; return true;      // Options
        case kBtnY: *key = Qt::Key_F; return true;      // Search
        case kBtnStart: *key = Qt::Key_O; return true;  // Open Prism
        case kBtnMenu: *key = Qt::Key_O; return true;
        case kBtnLb:
        case kBtnRb:
        case kBtnBack:
            *key = 0;
            return true;
        default: return false;
    }
}

void GamepadFilter::dispatchAction(int actionRaw) {
    const PadAction action = static_cast<PadAction>(actionRaw);
    switch (action) {
        case PadAction::PrevView: emit previousViewRequested(); break;
        case PadAction::NextView: emit nextViewRequested(); break;
        case PadAction::Options: emit optionsRequested(); break;
        case PadAction::Search: emit searchRequested(); break;
        case PadAction::Prism: emit prismRequested(); break;
        case PadAction::Back: emit backRequested(); break;
        case PadAction::Play:
        case PadAction::None:
            break;  // covered by the posted key events
    }
}

void GamepadFilter::handleEvent(PadState& pad, const js_event& ev) {
    const qint16 dz = deadzoneThreshold();

    if (ev.type == TYPE_BUTTON) {
        const int idx = ev.number;
        const bool pressed = ev.value != 0;
        if (idx >= pad.buttonStates.size()) {
            return;
        }
        const bool wasPressed = pad.buttonStates[idx];
        pad.buttonStates[idx] = pressed;
        if (!pressed || wasPressed) {
            if (!pressed && pad.heldKey != 0) {
                pad.heldKey = 0;  // release stops repeat
            }
            return;
        }

        int key = 0;
        if (!mapButton(idx, &key)) {
            return;
        }
        // One-shot semantic actions; never auto-repeat.
        switch (idx) {
            case kBtnLb: dispatchAction(static_cast<int>(PadAction::PrevView)); break;
            case kBtnRb: dispatchAction(static_cast<int>(PadAction::NextView)); break;
            case kBtnX: dispatchAction(static_cast<int>(PadAction::Options)); break;
            case kBtnY: dispatchAction(static_cast<int>(PadAction::Search)); break;
            case kBtnStart:
            case kBtnMenu: dispatchAction(static_cast<int>(PadAction::Prism)); break;
            case kBtnBack: dispatchAction(static_cast<int>(PadAction::Back)); break;
            case kBtnA: dispatchAction(static_cast<int>(PadAction::Play)); break;
            case kBtnB: dispatchAction(static_cast<int>(PadAction::Back)); break;
            default: break;
        }
        if (key == 0) {
            return;  // signal-only action (shoulders, Back)
        }
        // Escape/E/F/O are one-shot intents: no hold-repeat, no key spam.
        if (key == Qt::Key_Return) {
            pad.heldKey = key;
            pad.repeatTimer.restart();
            pad.repeatFired = false;
        }
        postKey(key, false);
        emit gamepadKey(key);
        return;
    }

    if (ev.type == TYPE_AXIS) {
        const int idx = ev.number;
        if (idx >= pad.axisStates.size()) {
            return;
        }
        pad.axisStates[idx] = ev.value;

        if (idx == AXIS_DPAD_X || idx == AXIS_LEFT_X) {
            const int key = ev.value < -dz ? Qt::Key_Left : (ev.value > dz ? Qt::Key_Right : 0);
            if (key != 0 && key != pad.heldKey) {
                pad.heldKey = key;
                pad.repeatTimer.restart();
                pad.repeatFired = false;
                postKey(key, false);
                emit gamepadKey(key);
            } else if (key == 0 && (pad.heldKey == Qt::Key_Left || pad.heldKey == Qt::Key_Right)) {
                pad.heldKey = 0;
            }
        } else if (idx == AXIS_DPAD_Y || idx == AXIS_LEFT_Y) {
            const int key = ev.value < -dz ? Qt::Key_Up : (ev.value > dz ? Qt::Key_Down : 0);
            if (key != 0 && key != pad.heldKey) {
                pad.heldKey = key;
                pad.repeatTimer.restart();
                pad.repeatFired = false;
                postKey(key, false);
                emit gamepadKey(key);
            } else if (key == 0 && (pad.heldKey == Qt::Key_Up || pad.heldKey == Qt::Key_Down)) {
                pad.heldKey = 0;
            }
        }
    }
}

void GamepadFilter::postKey(int key, bool autoRepeat) {
    QWidget* focus = QApplication::focusWidget();
    if (!focus) {
        return;
    }
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, QString(), autoRepeat);
    QApplication::sendEvent(focus, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, QString(), false);
    QApplication::sendEvent(focus, &release);
}
