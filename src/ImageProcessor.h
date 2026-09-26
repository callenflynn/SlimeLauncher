#pragma once

#include "Constants.h"
#include "InstanceCardModel.h"

#include <QDateTime>
#include <QString>
#include <QVector>

class QImage;

// Per-instance artwork pipeline.
//
// Owns the `slimelauncher/` asset folder inside every Prism instance dir:
//   <instance>/slimelauncher/card.png        — 2:3 poster cover (owned by Slime)
//   <instance>/slimelauncher/background.png  — optional hero wallpaper (owned by Slime)
//   <instance>/slimelauncher/metadata.json   — display overrides (owned by Slime)
//
// The folder is auto-created on ensureAssets(); a missing card.png is seeded
// from a deterministic per-instance pick of the embedded default posters, run
// through the 2:3 center-crop engine before it is written. Custom uploads via
// importCardArtwork() always pass through the same crop pipeline so every
// card on the grid is exactly 2:3.
class ImageProcessor {
public:
    // Location of the slimelauncher/ asset folder for one instance.
    static QString assetDirPath(const QString& instancesDir, const QString& instanceId);

    // Absolute path of card.png / background.png / metadata.json for one instance.
    static QString cardPath(const QString& instancesDir, const QString& instanceId);
    static QString backgroundPath(const QString& instancesDir, const QString& instanceId);
    static QString metadataPath(const QString& instancesDir, const QString& instanceId);

    // Center-crops `source` to 2:3 and scales it into the 300x450–600x900 px
    // window (Qt::KeepAspectRatioByExpanding). Returns a null image if the
    // source cannot be loaded.
    static QImage processToCardRatio(const QImage& source);

    // Loads an arbitrary image file and runs it through processToCardRatio().
    static QImage processFileToCardRatio(const QString& sourcePath);

    // Creates <instance>/slimelauncher/ when missing and writes card.png from
    // a default poster when no custom artwork exists yet. Returns the card
    // path on success, or an empty string when the pipeline cannot provision
    // the folder (read-only filesystem, undecodable defaults, ...).
    static QString ensureAssets(const QString& instancesDir, const InstanceCardModel& info);

    // Runs a user-selected image through the crop engine and saves it as that
    // instance's card.png, marking metadata artwork=custom. Returns success.
    static bool importCardArtwork(const QString& instancesDir, const QString& instanceId,
                                  const QString& sourceImagePath, QString* errorOut = nullptr);

    // Writes an already-processed image to card.png. Shared by the default
    // seeding and the custom-import paths.
    static bool saveCardImage(const QString& instancesDir, const QString& instanceId,
                              const QImage& card, QString* errorOut = nullptr);

    // True when card.png exists for this instance (i.e. user custom artwork
    // or a previously seeded default).
    static bool hasCardAsset(const QString& instancesDir, const QString& instanceId);

    // Deterministic default-card index for an instance (stable across
    // rescans: seeded from the instance id so cards do not reshuffle).
    static int defaultCardIndex(const QString& instanceId);

private:
    // Picks, crops and returns one of the embedded default posters.
    static QImage buildDefaultCard(const QString& instanceId);

    static bool writeMetadata(const QString& instancesDir, const QString& instanceId,
                              const QString& artworkKind, const QString& sourceNote);
};
