#include "PlaylistManager.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <algorithm>

PlaylistManager::PlaylistManager(QObject *parent) : QObject(parent) {}

bool PlaylistManager::isAudioFile(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QSet<QString> ok = {"mp3","wav","flac","ogg","aac","m4a","wma","opus","aiff"};
    return ok.contains(ext);
}

void PlaylistManager::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_tracks.size()) return;
    if (m_current == index) return;
    m_current = index;
    emit currentChanged(index);
}

QUrl PlaylistManager::urlAt(int index) const
{
    if (index < 0 || index >= m_tracks.size()) return {};
    return QUrl::fromLocalFile(m_tracks[index].path);
}

QStringList PlaylistManager::paths() const
{
    QStringList out;
    for (const auto &t : m_tracks) out << t.path;
    return out;
}

qint64 PlaylistManager::totalDuration() const
{
    qint64 sum = 0;
    for (const auto &t : m_tracks) sum += t.duration;
    return sum;
}

void PlaylistManager::clear()
{
    m_tracks.clear();
    m_current = -1;
    emit changed();
    emit currentChanged(-1);
}

void PlaylistManager::addFiles(const QStringList &files, bool replace)
{
    if (replace) clear();
    for (const QString &path : files) {
        if (!isAudioFile(path)) continue;
        QFileInfo info(path);
        TrackItem t;
        t.path = info.absoluteFilePath();
        t.title = info.completeBaseName();
        t.artist = "Unknown Artist";
        t.album = "Unknown Album";
        t.added = QDateTime::currentDateTime();
        m_tracks.append(t);
    }
    emit changed();
}

void PlaylistManager::removeRows(const QList<int> &rows)
{
    QList<int> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int r : sorted) if (r >= 0 && r < m_tracks.size()) m_tracks.removeAt(r);
    if (m_current >= m_tracks.size()) m_current = m_tracks.isEmpty() ? -1 : 0;
    emit changed();
    emit currentChanged(m_current);
}

void PlaylistManager::removeDuplicates()
{
    QSet<QString> seen;
    QList<TrackItem> unique;
    for (const auto &t : m_tracks) {
        if (seen.contains(t.path)) continue;
        seen.insert(t.path);
        unique << t;
    }
    m_tracks = unique;
    emit changed();
}

bool PlaylistManager::saveToFile(const QString &fileName) const
{
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    for (const auto &t : m_tracks) out << t.path << '\n';
    return true;
}

bool PlaylistManager::loadFromFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QStringList files;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty()) files << line;
    }
    addFiles(files, true);
    return true;
}
