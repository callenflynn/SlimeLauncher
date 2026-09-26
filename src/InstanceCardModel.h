#pragma once

#include "Constants.h"

#include <QDateTime>
#include <QMetaType>
#include <QString>

// Parsed view of one Prism instance — shared by backend and UI widgets.
// PrismBridge builds these read-only from Prism's own metadata files; cardPath
// points at the Slime-owned 2:3 poster in the instance's slimelauncher/ folder.
struct InstanceCardModel {
    QString id;
    QString name;
    QString gameVersion;
    QString loader;
    QString iconPath;
    QString cardPath;
    QDateTime lastPlayed;
    bool playing = false;
    bool valid = true;
};

Q_DECLARE_METATYPE(InstanceCardModel)
