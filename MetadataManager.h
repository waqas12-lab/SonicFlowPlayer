#ifndef METADATAMANAGER_H
#define METADATAMANAGER_H
#include <QObject>
#include <QMediaMetaData>
#include <QPixmap>
#include "PlaylistManager.h"

class MetadataManager : public QObject
{
    Q_OBJECT
public:
    explicit MetadataManager(QObject *parent = nullptr);
    void applyMetadata(TrackItem &track, const QMediaMetaData &meta) const;
    QPixmap artworkFromMeta(const QMediaMetaData &meta, int size = 320) const;
    QPixmap defaultArtwork(int size = 320) const;
};
#endif
