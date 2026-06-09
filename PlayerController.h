#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

class PlayerController : public QObject
{
    Q_OBJECT
public:
    enum RepeatMode { RepeatOff, RepeatOne, RepeatAll };
    Q_ENUM(RepeatMode)

    explicit PlayerController(QObject *parent = nullptr);
    QMediaPlayer *player() { return &m_player; }
    int volume() const { return m_volume; }
    bool isMuted() const { return m_audio.isMuted(); }
    bool shuffle() const { return m_shuffle; }
    RepeatMode repeatMode() const { return m_repeat; }
    qint64 duration() const { return m_player.duration(); }
    qint64 position() const { return m_player.position(); }

public slots:
    void playUrl(const QUrl &url);
    void play();
    void pause();
    void stop();
    void togglePlayPause();
    void setPosition(qint64 pos);
    void seekBy(qint64 deltaMs);
    void setVolume(int percent);
    void volumeUp();
    void volumeDown();
    void toggleMute();
    void toggleShuffle();
    void cycleRepeat();
    void setPlaybackRate(qreal rate);

signals:
    void playbackStateChanged(QMediaPlayer::PlaybackState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void errorMessage(const QString &message);
    void volumeChanged(int percent);
    void muteChanged(bool muted);
    void shuffleChanged(bool enabled);
    void repeatChanged(PlayerController::RepeatMode mode);

private:
    QMediaPlayer m_player;
    QAudioOutput m_audio;
    int m_volume = 40;
    bool m_shuffle = false;
    RepeatMode m_repeat = RepeatOff;
};

#endif
