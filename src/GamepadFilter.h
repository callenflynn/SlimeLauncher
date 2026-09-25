#pragma once

#include "Constants.h"

#include <QAbstractNativeEventFilter>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

#include <linux/joystick.h>

// Application-wide gamepad → keyboard event filter.
//
// Reads /dev/input/js* directly (no SDL2 dependency) through low-level fds
// pumped by a poll timer, and translates gamepad input into synthetic Qt key
// events posted to the focused widget, so gamepad and keyboard share one
// focus model.
//
// Mapping:
//   A (js button 0)      → Enter
//   B (js button 1)      → Escape
//   X (js button 2)      → F5
//   Y (js button 3)      → Tab
//   Start (js button 9)  → F5
//   D-pad / left stick   → Arrow keys
//
// Deadzone 35%, hold-repeat with acceleration after 450 ms.
class GamepadFilter : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GamepadFilter(QObject* parent = nullptr);
    ~GamepadFilter() override;

    // QAbstractNativeEventFilter — pumps device reads on XCB events.
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void gamepadKey(int key);

private:
    struct PadState {
        int fd = -1;
        QString devicePath;
        QVector<bool> buttonStates;
        QVector<qint16> axisStates;
        QElapsedTimer repeatTimer;
        int heldKey = 0;
        bool repeatFired = false;
    };

    void scanDevices();
    void closeAll();
    void pump();
    void handleEvent(PadState& pad, const js_event& ev);
    void postKey(int key, bool autoRepeat);

    QHash<int, PadState*> m_pads;
    QTimer* m_pollTimer = nullptr;
};
