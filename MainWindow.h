#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QShortcut>
#include <QToolButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QMediaPlayer>
#include "PlayerController.h"
#include "PlaylistManager.h"
#include "MetadataManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void openFiles();
    void openFolder();
    void addFiles();
    void savePlaylist();
    void loadPlaylist();
    void removeSelected();
    void clearPlaylist();
    void removeDuplicates();
    void playSelectedRow();
    void playIndex(int index);
    void playNext();
    void playPrevious();
    void updatePlaylist();
    void updateNowPlaying();
    void updatePlaybackState(QMediaPlayer::PlaybackState state);
    void updatePosition(qint64 pos);
    void updateDuration(qint64 duration);
    void handleMediaStatus(QMediaPlayer::MediaStatus status);
    void updateVolumeLabel(int volume);
    void updateMuteState(bool muted);
    void updateShuffleState(bool enabled);
    void updateRepeatState(PlayerController::RepeatMode mode);
    void showHelp();
    void showAbout();
    void showLyrics();
    void filterRows(const QString &text);

private:
    void buildUi();
    void buildMenus();
    void buildToolbar();
    void buildShortcuts();
    void buildTray();
    void applyTheme();
    QWidget *buildNowPlayingPanel();
    QWidget *buildPlaylistPanel();
    QWidget *buildControlBar();
    QPushButton *controlButton(const QString &icon, const QString &title, const QString &shortcut, bool primary = false);
    QToolButton *toolButton(const QString &icon, const QString &text, const QString &tip);
    void addFilesInternal(const QStringList &files, bool replace, bool autoPlay);
    QStringList audioFilesFromFolder(const QString &folder) const;
    QString formatTime(qint64 ms) const;
    QString shortcutTip(const QString &name, const QString &keys) const;
    void saveSettings();
    void loadSettings();
    void setArtwork(const QPixmap &pixmap);
    void setSongInfo(const TrackItem &track);
    int selectedRow() const;
    QString repeatLabel() const;

    PlayerController m_player;
    PlaylistManager m_playlist;
    MetadataManager m_metadata;
    QSettings m_settings;

    QWidget *m_central = nullptr;
    QSplitter *m_splitter = nullptr;
    QWidget *m_toolbar = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_artwork = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_artist = nullptr;
    QLabel *m_album = nullptr;
    QLabel *m_authors = nullptr;
    QLabel *m_format = nullptr;
    QLabel *m_lyricsText = nullptr;
    QLabel *m_leftTime = nullptr;
    QLabel *m_rightTime = nullptr;
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusCenter = nullptr;
    QLabel *m_volumePercent = nullptr;
    QSlider *m_seek = nullptr;
    QSlider *m_volume = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_speed = nullptr;
    QPushButton *m_playPause = nullptr;
    QPushButton *m_shuffle = nullptr;
    QPushButton *m_repeat = nullptr;
    QPushButton *m_mute = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    qint64 m_duration = 0;
    bool m_seeking = false;
};

#endif
