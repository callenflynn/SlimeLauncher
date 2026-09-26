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

// Application-wide gamepad → console action filter.
//
// Reads /dev/input/js* directly (no SDL2 dependency) through low-level fds
// pumped by a poll timer. Face buttons and shoulders become synthetic Qt key
// events posted to the focus widget (shared focus model); the shoulders and
// auxiliary buttons additionally emit dedicated console signals that the
// dashboard binds to view switching, the options menu, the search overlay,
// and the Prism fallback.
//
// Mapping (Linux joystick API button indices):
//   A (0)                → Enter (+ pressed signal)
//   B (1)                → Escape (+ back signal)
//   X (2)                → Key_E → Options context menu signal
//   Y (3)                → Key_F → Search overlay signal
//   LB (4) / RB (5)      → view-switch signal (Prev/Next), no key posted
//   Back (8)             → back signal
//   Start/Menu (9, 10)   → Key_O → open-Prism signal
//   D-pad / left stick   → Arrow keys
//
// Deadzone 35%, hold-repeat with acceleration after 450 ms (face buttons and
// arrows only — shoulders/menu actions fire once per press).
class GamepadFilter : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GamepadFilter(QObject* parent = nullptr);
    ~GamepadFilter() override;

    // QAbstractNativeEventFilter — pumps device reads on XCB events.
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void gamepadKey(int key);

    // Console semantic actions (emitted in addition to the key mapping).
    void previousViewRequested();
    void nextViewRequested();
    void optionsRequested();
    void searchRequested();
    void prismRequested();
    void backRequested();

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

    // Maps a pressed button index to a key + console action pair. Returns
    // false for unmapped indices.
    static bool mapButton(int index, int* key);

    void dispatchAction(int action);

    QHash<int, PadState*> m_pads;
    QTimer* m_pollTimer = nullptr;
};
