#include "ImageProcessor.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

namespace {

// 2:3 poster window: min 300x450, max 600x900 (per the asset spec).
constexpr int CARD_MIN_W = 300;
constexpr int CARD_MIN_H = 450;
constexpr int CARD_MAX_W = 600;
constexpr int CARD_MAX_H = 900;

QString assetDir(const QString& instancesDir, const QString& instanceId) {
    return QDir(instancesDir).filePath(instanceId) + QLatin1Char('/') + QLatin1String(Constants::SLIME_DIR_NAME);
}

QImage loadImage(const QString& path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    return reader.read();
}

}  // namespace

QString ImageProcessor::assetDirPath(const QString& instancesDir, const QString& instanceId) {
    return assetDir(instancesDir, instanceId);
}

QString ImageProcessor::cardPath(const QString& instancesDir, const QString& instanceId) {
    return assetDir(instancesDir, instanceId) + QLatin1Char('/') + QLatin1String(Constants::CARD_ASSET_NAME);
}

QString ImageProcessor::backgroundPath(const QString& instancesDir, const QString& instanceId) {
    return assetDir(instancesDir, instanceId) + QLatin1Char('/') + QLatin1String(Constants::BACKGROUND_ASSET_NAME);
}

QString ImageProcessor::metadataPath(const QString& instancesDir, const QString& instanceId) {
    return assetDir(instancesDir, instanceId) + QLatin1Char('/') + QLatin1String(Constants::METADATA_ASSET_NAME);
}

QImage ImageProcessor::processToCardRatio(const QImage& source) {
    if (source.isNull()) {
        return {};
    }

    // Normalize EXIF orientation before measuring.
    QImage image = source;

    const qreal target = 2.0 / 3.0;  // width / height
    const qreal actual = qreal(image.width()) / qreal(qMax(1, image.height()));

    QImage cropped;
    if (qFuzzyCompare(actual, target)) {
        cropped = image;
    } else if (actual > target) {
        // Too wide: crop the sides, keep the vertical center.
        const int keepW = int(qRound(qreal(image.height()) * target));
        const int x = (image.width() - keepW) / 2;
        cropped = image.copy(x, 0, keepW, image.height());
    } else {
        // Too tall: crop the top/bottom, keep the horizontal center.
        const int keepH = int(qRound(qreal(image.width()) / target));
        const int y = (image.height() - keepH) / 2;
        cropped = image.copy(0, y, image.width(), keepH);
    }

    // The crop above already produces an exact 2:3 image, so scaling is a
    // plain clamp into the 450–900 px window. The height snaps to the nearest
    // multiple of 3 so width = 2*height/3 is an exact integer pair (no 1px
    // aspect drift from rounding).
    const int k = qBound(CARD_MIN_H / 3, qRound(cropped.height() / 3.0), CARD_MAX_H / 3);
    const int targetH = 3 * k;
    const int targetW = 2 * k;
    if (targetH == cropped.height() && targetW == cropped.width()) {
        return cropped;
    }
    return cropped.scaled(QSize(targetW, targetH), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage ImageProcessor::processFileToCardRatio(const QString& sourcePath) {
    return processToCardRatio(loadImage(sourcePath));
}

int ImageProcessor::defaultCardIndex(const QString& instanceId) {
    // Deterministic per-instance assignment: a hash-seeded uniform pick over
    // the embedded posters. Stable across rescans so a given instance always
    // lands on the same default card until the user overrides it.
    const QByteArray digest =
        QCryptographicHash::hash(instanceId.toUtf8(), QCryptographicHash::Sha256);
    const quint32 seed = qFromBigEndian<quint32>(digest.constData());
    return int(seed % quint32(Constants::DEFAULT_CARD_COUNT));
}

QImage ImageProcessor::buildDefaultCard(const QString& instanceId) {
    const int index = defaultCardIndex(instanceId);
    if (index < 0 || index >= Constants::DEFAULT_CARD_COUNT) {
        return {};
    }
    const QImage raw = loadImage(QLatin1String(Constants::DEFAULT_CARDS[index]));
    return processToCardRatio(raw);
}

bool ImageProcessor::hasCardAsset(const QString& instancesDir, const QString& instanceId) {
    return QFileInfo::exists(cardPath(instancesDir, instanceId));
}

QString ImageProcessor::ensureAssets(const QString& instancesDir, const InstanceCardModel& info) {
    if (instancesDir.isEmpty() || info.id.isEmpty()) {
        return QString();
    }
    QDir dir(assetDir(instancesDir, info.id));
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        return QString();
    }

    const QString card = cardPath(instancesDir, info.id);
    if (!QFileInfo::exists(card)) {
        const QImage seeded = buildDefaultCard(info.id);
        if (seeded.isNull()) {
            return QString();
        }
        QString error;
        if (!saveCardImage(instancesDir, info.id, seeded, &error)) {
            return QString();
        }
        writeMetadata(instancesDir, info.id, QLatin1String(Constants::METADATA_ARTWORK_DEFAULT),
                      QStringLiteral("default poster %1").arg(defaultCardIndex(info.id) + 1));
    }
    return card;
}

bool ImageProcessor::importCardArtwork(const QString& instancesDir, const QString& instanceId,
                                       const QString& sourceImagePath, QString* errorOut) {
    if (errorOut) {
        errorOut->clear();
    }
    if (instancesDir.isEmpty() || instanceId.isEmpty() || sourceImagePath.isEmpty()) {
        if (errorOut) {
            *errorOut = QLatin1String("Missing instance or image path.");
        }
        return false;
    }
    QDir dir(assetDir(instancesDir, instanceId));
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        if (errorOut) {
            *errorOut = QStringLiteral("Cannot create asset folder: %1").arg(dir.absolutePath());
        }
        return false;
    }

    QImageReader reader(sourceImagePath);
    reader.setAutoTransform(true);
    const QImage source = reader.read();
    if (source.isNull()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Unsupported or unreadable image: %1").arg(sourceImagePath);
        }
        return false;
    }

    const QImage card = processToCardRatio(source);
    if (card.isNull()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Image processing failed: %1").arg(sourceImagePath);
        }
        return false;
    }
    if (!saveCardImage(instancesDir, instanceId, card, errorOut)) {
        return false;
    }
    return writeMetadata(instancesDir, instanceId, QLatin1String(Constants::METADATA_ARTWORK_CUSTOM),
                         QFileInfo(sourceImagePath).fileName());
}

bool ImageProcessor::saveCardImage(const QString& instancesDir, const QString& instanceId,
                                   const QImage& card, QString* errorOut) {
    if (errorOut) {
        errorOut->clear();
    }
    QDir dir(assetDir(instancesDir, instanceId));
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        if (errorOut) {
            *errorOut = QStringLiteral("Cannot create asset folder: %1").arg(dir.absolutePath());
        }
        return false;
    }
    const QString target = cardPath(instancesDir, instanceId);
    if (!card.save(target, "PNG")) {
        if (errorOut) {
            *errorOut = QStringLiteral("Cannot write card image: %1").arg(target);
        }
        return false;
    }
    return true;
}

bool ImageProcessor::writeMetadata(const QString& instancesDir, const QString& instanceId,
                                   const QString& artworkKind, const QString& sourceNote) {
    QJsonObject root;
    root.insert(QLatin1String(Constants::METADATA_ARTWORK_KEY), artworkKind);
    root.insert(QLatin1String(Constants::METADATA_SOURCE_KEY), sourceNote);
    root.insert(QLatin1String(Constants::METADATA_UPDATED_KEY),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

    QDir dir(assetDir(instancesDir, instanceId));
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        return false;
    }
    QFile file(metadataPath(instancesDir, instanceId));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0;
}
