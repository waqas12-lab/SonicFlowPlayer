#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QDateTime>

struct TrackItem {
    QString path;
    QString title;
    QString artist;
    QString album;
    qint64 duration = 0;
    QDateTime added;
};

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    const QList<TrackItem> &tracks() const { return m_tracks; }
    int count() const { return m_tracks.size(); }
    TrackItem track(int index) const { return m_tracks.value(index); }
    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);
    QUrl urlAt(int index) const;
    QStringList paths() const;
    qint64 totalDuration() const;

public slots:
    void clear();
    void addFiles(const QStringList &files, bool replace = false);
    void removeRows(const QList<int> &rows);
    void removeDuplicates();
    bool saveToFile(const QString &fileName) const;
    bool loadFromFile(const QString &fileName);

signals:
    void changed();
    void currentChanged(int index);

private:
    static bool isAudioFile(const QString &path);
    QList<TrackItem> m_tracks;
    int m_current = -1;
};

#endif
