#include "PlayerController.h"
#include <QtGlobal>

PlayerController::PlayerController(QObject *parent) : QObject(parent)
{
    m_player.setAudioOutput(&m_audio);
    setVolume(m_volume);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &PlayerController::playbackStateChanged);
    connect(&m_player, &QMediaPlayer::positionChanged, this, &PlayerController::positionChanged);
    connect(&m_player, &QMediaPlayer::durationChanged, this, &PlayerController::durationChanged);
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, &PlayerController::mediaStatusChanged);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &text){
        if (!text.isEmpty()) emit errorMessage(text);
    });
}

void PlayerController::playUrl(const QUrl &url) { m_player.setSource(url); m_player.play(); }
void PlayerController::play() { m_player.play(); }
void PlayerController::pause() { m_player.pause(); }
void PlayerController::stop() { m_player.stop(); }
void PlayerController::togglePlayPause() { m_player.playbackState() == QMediaPlayer::PlayingState ? m_player.pause() : m_player.play(); }
void PlayerController::setPosition(qint64 pos) { m_player.setPosition(qBound<qint64>(0, pos, m_player.duration())); }
void PlayerController::seekBy(qint64 deltaMs) { setPosition(m_player.position() + deltaMs); }
void PlayerController::setVolume(int percent) { m_volume = qBound(0, percent, 100); m_audio.setVolume(m_volume / 100.0); emit volumeChanged(m_volume); }
void PlayerController::volumeUp() { setVolume(m_volume + 5); }
void PlayerController::volumeDown() { setVolume(m_volume - 5); }
void PlayerController::toggleMute() { m_audio.setMuted(!m_audio.isMuted()); emit muteChanged(m_audio.isMuted()); }
void PlayerController::toggleShuffle() { m_shuffle = !m_shuffle; emit shuffleChanged(m_shuffle); }
void PlayerController::cycleRepeat() { m_repeat = m_repeat == RepeatOff ? RepeatAll : (m_repeat == RepeatAll ? RepeatOne : RepeatOff); emit repeatChanged(m_repeat); }
void PlayerController::setPlaybackRate(qreal rate) { m_player.setPlaybackRate(rate); }
