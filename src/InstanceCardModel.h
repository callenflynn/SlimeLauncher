#pragma once

#include "Constants.h"

#include <QDateTime>
#include <QMetaType>
#include <QString>

// Parsed view of one Prism instance — shared by backend and UI widgets.
// PrismBridge builds these read-only from Prism's own metadata files.
struct InstanceCardModel {
    QString id;
    QString name;
    QString gameVersion;
    QString loader;
    QString iconPath;
    QDateTime lastPlayed;
    bool playing = false;
    bool valid = true;
};

Q_DECLARE_METATYPE(InstanceCardModel)
