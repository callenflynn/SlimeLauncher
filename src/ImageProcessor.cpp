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
#include <QPainter>
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

// Cheap stack-blur approximation: repeated box blurs converge on a Gaussian
// and avoid a QImageConvolutionMatrix dependency.
QImage boxBlur(QImage src, int radius) {
    radius = qBound(1, radius, 32);
    QImage src2 = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage dst(src2.size(), QImage::Format_ARGB32_Premultiplied);
    for (int pass = 0; pass < 3; ++pass) {
        // Horizontal pass
        for (int y = 0; y < src2.height(); ++y) {
            const QRgb* in = reinterpret_cast<const QRgb*>(src2.constScanLine(y));
            QRgb* out = reinterpret_cast<QRgb*>(dst.scanLine(y));
            int r = 0, g = 0, b = 0;
            const int win = radius * 2 + 1;
            for (int x = -radius; x <= radius && x < src2.width(); ++x) {
                const QRgb px = in[qBound(0, x, src2.width() - 1)];
                r += qRed(px); g += qGreen(px); b += qBlue(px);
            }
            for (int x = 0; x < src2.width(); ++x) {
                const int addIdx = qMin(src2.width() - 1, x + radius + 1);
                const int subIdx = qMax(0, x - radius);
                const QRgb add = in[addIdx];
                const QRgb sub = in[subIdx];
                r += qRed(add) - qRed(sub);
                g += qGreen(add) - qGreen(sub);
                b += qBlue(add) - qBlue(sub);
                out[x] = qRgb(r / win, g / win, b / win);
            }
        }
        // Vertical pass
        QImage tmp = dst;
        for (int y = 0; y < tmp.height(); ++y) {
            QRgb* out = reinterpret_cast<QRgb*>(dst.scanLine(y));
            for (int x = 0; x < tmp.width(); ++x) {
                int r = 0, g = 0, b = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    const int yy = qBound(0, y + dy, tmp.height() - 1);
                    const QRgb px = reinterpret_cast<const QRgb*>(tmp.constScanLine(yy))[x];
                    r += qRed(px); g += qGreen(px); b += qBlue(px);
                }
                const int win = radius * 2 + 1;
                out[x] = qRgb(r / win, g / win, b / win);
            }
        }
    }
    return dst;
}

QImage applyBlur(const QImage& source, qreal radius) {
    return boxBlur(source, int(radius));
}

QImage applyScrim(const QImage& source, qreal opacity) {
    QImage out = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(&out);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.fillRect(out.rect(), QColor(10, 10, 14, int(255 * qBound(0.0, opacity, 1.0))));
    p.end();
    return out;
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

QImage ImageProcessor::heroBackdropImage(const QString& instancesDir, const QString& instanceId,
                                         const QSize& targetSize) {
    if (instancesDir.isEmpty() || instanceId.isEmpty()) {
        return {};
    }

    // 1. Explicit hero wallpaper ships unblurred.
    const QImage background = loadImage(backgroundPath(instancesDir, instanceId));
    if (!background.isNull()) {
        return background;
    }

    // 2. Derive from card artwork: blur + darken so the poster grid and UI
    //    text stay legible over it.
    QImage card = loadImage(cardPath(instancesDir, instanceId));
    if (card.isNull()) {
        const int index = defaultCardIndex(instanceId);
        if (index >= 0 && index < Constants::DEFAULT_CARD_COUNT) {
            card = loadImage(QLatin1String(Constants::DEFAULT_CARDS[index]));
        }
    }
    if (card.isNull()) {
        return {};
    }

    QSize size = targetSize;
    if (size.isEmpty() || size.width() <= 0 || size.height() <= 0) {
        size = QSize(1280, 720);
    }
    // Scale to cover the target, blur, then darken so UI text stays legible.
    QImage cover = card.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int cw = qMin(cover.width(), size.width());
    const int ch = qMin(cover.height(), size.height());
    cover = cover.copy((cover.width() - cw) / 2, (cover.height() - ch) / 2, cw, ch);

    const qreal radius = qMax(qreal(8.0), qMax(cw, ch) / 48.0);
    const QImage blurred = applyBlur(cover, radius);
    return applyScrim(blurred, 0.45);
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
