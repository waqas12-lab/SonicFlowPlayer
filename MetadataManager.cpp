#include "MetadataManager.h"
#include <QPainter>
#include <QFileInfo>

MetadataManager::MetadataManager(QObject *parent) : QObject(parent) {}

void MetadataManager::applyMetadata(TrackItem &track, const QMediaMetaData &meta) const
{
    const QString title = meta.stringValue(QMediaMetaData::Title);
    const QString artist = meta.stringValue(QMediaMetaData::ContributingArtist);
    const QString album = meta.stringValue(QMediaMetaData::AlbumTitle);
    if (!title.isEmpty()) track.title = title;
    if (!artist.isEmpty()) track.artist = artist;
    if (!album.isEmpty()) track.album = album;
}

QPixmap MetadataManager::artworkFromMeta(const QMediaMetaData &meta, int size) const
{
    QVariant v = meta.value(QMediaMetaData::ThumbnailImage);
    if (!v.isValid()) v = meta.value(QMediaMetaData::CoverArtImage);
    QImage img = v.value<QImage>();
    if (!img.isNull()) return QPixmap::fromImage(img).scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    return defaultArtwork(size);
}

QPixmap MetadataManager::defaultArtwork(int size) const
{
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, size, size);
    g.setColorAt(0, QColor("#0b1d2d"));
    g.setColorAt(1, QColor("#0b69d8"));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(px.rect().adjusted(2,2,-2,-2), 24, 24);
    p.setPen(QPen(QColor("#dff4ff"), 10));
    QFont f = p.font(); f.setPixelSize(size / 2); f.setBold(true); p.setFont(f);
    p.drawText(px.rect(), Qt::AlignCenter, QStringLiteral("♪"));
    return px;
}
