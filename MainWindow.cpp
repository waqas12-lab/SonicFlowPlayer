#include "MainWindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QRandomGenerator>
#include <QStatusBar>
#include <QStyle>
#include <QUrl>
#include <QFileInfo>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_settings("SonicFlow", "SonicFlowPlayer")
{
    setWindowTitle("SonicFlow Player");
    setWindowIcon(QIcon(":/assets/assets/app_icon.png"));
    setAcceptDrops(true);
    resize(1280, 760);
    setMinimumSize(1120, 680);
    qApp->installEventFilter(this);

    buildUi();
    buildMenus();
    buildToolbar();
    buildShortcuts();
    buildTray();
    applyTheme();
    loadSettings();

    connect(&m_playlist, &PlaylistManager::changed, this, &MainWindow::updatePlaylist);
    connect(&m_playlist, &PlaylistManager::currentChanged, this, &MainWindow::updateNowPlaying);
    connect(&m_player, &PlayerController::playbackStateChanged, this, &MainWindow::updatePlaybackState);
    connect(&m_player, &PlayerController::positionChanged, this, &MainWindow::updatePosition);
    connect(&m_player, &PlayerController::durationChanged, this, &MainWindow::updateDuration);
    connect(&m_player, &PlayerController::mediaStatusChanged, this, &MainWindow::handleMediaStatus);
    connect(&m_player, &PlayerController::volumeChanged, this, &MainWindow::updateVolumeLabel);
    connect(&m_player, &PlayerController::muteChanged, this, &MainWindow::updateMuteState);
    connect(&m_player, &PlayerController::shuffleChanged, this, &MainWindow::updateShuffleState);
    connect(&m_player, &PlayerController::repeatChanged, this, &MainWindow::updateRepeatState);
    connect(&m_player, &PlayerController::errorMessage, this, [this](const QString &m){ statusBar()->showMessage(m, 6000); });
    connect(m_player.player(), &QMediaPlayer::metaDataChanged, this, &MainWindow::updateNowPlaying);

    updatePlaylist();
    updateNowPlaying();
    updateVolumeLabel(m_player.volume());
    updateShuffleState(m_player.shuffle());
    updateRepeatState(m_player.repeatMode());
    updateMuteState(m_player.isMuted());
}

void MainWindow::buildUi()
{
    m_central = new QWidget(this);
    setCentralWidget(m_central);
    auto *root = new QVBoxLayout(m_central);
    root->setContentsMargins(18, 14, 18, 0);
    root->setSpacing(0);

    auto *titleRow = new QHBoxLayout;
    titleRow->setSpacing(10);
    auto *appTitle = new QLabel("SonicFlow Player");
    appTitle->setObjectName("windowTitle");
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search...");
    m_search->setClearButtonEnabled(true);
    m_search->setFixedWidth(340);
    m_search->setToolTip(shortcutTip("Search", "Ctrl+F / Cmd+F"));
    titleRow->addStretch(1);
    titleRow->addWidget(appTitle, 0, Qt::AlignCenter);
    titleRow->addStretch(1);
    titleRow->addWidget(m_search);
    root->addLayout(titleRow);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::filterRows);

    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("separator");
    root->addWidget(line);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setObjectName("mainSplitter");
    m_splitter->setChildrenCollapsible(false);
    m_splitter->addWidget(buildPlaylistPanel());
    m_splitter->addWidget(buildNowPlayingPanel());
    m_splitter->setStretchFactor(0, 56);
    m_splitter->setStretchFactor(1, 44);
    m_splitter->setSizes({710, 570});
    root->addWidget(m_splitter, 1);

    root->addWidget(buildControlBar());

    statusBar()->setObjectName("status");
    m_statusLeft = new QLabel("0 tracks");
    m_statusCenter = new QLabel("Ready");
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addWidget(m_statusCenter, 2);
}

void MainWindow::buildMenus()
{
    auto *file = menuBar()->addMenu("File");
    file->addAction("Open File", QKeySequence::Open, this, &MainWindow::openFiles);
    file->addAction("Open Folder", QKeySequence("Ctrl+Shift+O"), this, &MainWindow::openFolder);
    file->addAction("Add to Playlist", this, &MainWindow::addFiles);
    file->addSeparator();
    file->addAction("Save Playlist", this, &MainWindow::savePlaylist);
    file->addAction("Load Playlist", this, &MainWindow::loadPlaylist);
    file->addSeparator();
    file->addAction("Quit", QKeySequence::Quit, qApp, &QApplication::quit);

    auto *play = menuBar()->addMenu("Playback");
    play->addAction("Play / Pause", &m_player, &PlayerController::togglePlayPause);
    play->addAction("Next Track", this, &MainWindow::playNext);
    play->addAction("Previous Track", this, &MainWindow::playPrevious);
    play->addAction("Seek Forward", this, [this]{ m_player.seekBy(10000); });
    play->addAction("Seek Back", this, [this]{ m_player.seekBy(-10000); });
    play->addAction("Mute", &m_player, &PlayerController::toggleMute);
    play->addAction("Shuffle", &m_player, &PlayerController::toggleShuffle);
    play->addAction("Repeat", &m_player, &PlayerController::cycleRepeat);

    auto *view = menuBar()->addMenu("View");
    view->addAction("Focus Search", QKeySequence::Find, m_search, qOverload<>(&QWidget::setFocus));

    auto *tools = menuBar()->addMenu("Tools");
    tools->addAction("Remove Selected", QKeySequence::Delete, this, &MainWindow::removeSelected);
    tools->addAction("Remove Duplicates", this, &MainWindow::removeDuplicates);
    tools->addAction("Clear Playlist", this, &MainWindow::clearPlaylist);

    auto *help = menuBar()->addMenu("Help");
    help->addAction("Keyboard Shortcuts", this, &MainWindow::showHelp);
    help->addAction("About SonicFlow Player", this, &MainWindow::showAbout);
}

void MainWindow::buildToolbar()
{
    m_toolbar = new QWidget;
    m_toolbar->setObjectName("toolbar");
    auto *l = new QHBoxLayout(m_toolbar);
    l->setContentsMargins(0, 10, 0, 10);
    l->setSpacing(32);
    l->addWidget(toolButton("📂", "Open File", shortcutTip("Open File", "Ctrl+O / Cmd+O")));
    l->addWidget(toolButton("📁", "Open Folder", shortcutTip("Open Folder", "Ctrl+Shift+O / Cmd+Shift+O")));
    l->addWidget(toolButton("⊕", "Add to Playlist", "Add files without interrupting current playback"));
    l->addWidget(toolButton("🗑", "Remove", shortcutTip("Remove Selected", "Delete")));
    l->addWidget(toolButton("×", "Clear", "Clear playlist"));
    l->addStretch();
    qobject_cast<QVBoxLayout*>(m_central->layout())->insertWidget(2, m_toolbar);
    const auto buttons = m_toolbar->findChildren<QToolButton*>();
    connect(buttons[0], &QToolButton::clicked, this, &MainWindow::openFiles);
    connect(buttons[1], &QToolButton::clicked, this, &MainWindow::openFolder);
    connect(buttons[2], &QToolButton::clicked, this, &MainWindow::addFiles);
    connect(buttons[3], &QToolButton::clicked, this, &MainWindow::removeSelected);
    connect(buttons[4], &QToolButton::clicked, this, &MainWindow::clearPlaylist);
}

QToolButton *MainWindow::toolButton(const QString &icon, const QString &text, const QString &tip)
{
    auto *b = new QToolButton;
    b->setText(icon + "  " + text);
    b->setToolTip(tip);
    b->setCursor(Qt::PointingHandCursor);
    b->setAutoRaise(true);
    b->setMinimumHeight(34);
    b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return b;
}

QWidget *MainWindow::buildPlaylistPanel()
{
    auto *wrap = new QWidget;
    wrap->setObjectName("playlistPanel");
    auto *layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 12, 0);
    layout->setSpacing(0);

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({"#", "Title", "Artist", "Album", "Duration"});
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(false);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(false);
    m_table->setWordWrap(false);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->setColumnWidth(0, 58);
    m_table->setColumnWidth(1, 260);
    m_table->setColumnWidth(2, 180);
    m_table->setColumnWidth(3, 180);
    m_table->setColumnWidth(4, 90);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row){ playIndex(row); });
    layout->addWidget(m_table);
    return wrap;
}

QWidget *MainWindow::buildNowPlayingPanel()
{
    auto *wrap = new QWidget;
    wrap->setObjectName("nowPanel");
    auto *layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(22, 12, 0, 12);
    layout->setSpacing(12);

    auto *tabs = new QHBoxLayout;
    auto *now = new QLabel("Now Playing");
    now->setObjectName("tabActive");
    auto *lyrics = new QPushButton("Lyrics");
    lyrics->setObjectName("tabButton");
    lyrics->setCursor(Qt::PointingHandCursor);
    lyrics->setToolTip("Open lyrics viewer");
    connect(lyrics, &QPushButton::clicked, this, &MainWindow::showLyrics);
    tabs->addWidget(now);
    tabs->addSpacing(48);
    tabs->addWidget(lyrics);
    tabs->addStretch();
    layout->addLayout(tabs);

    auto *content = new QHBoxLayout;
    content->setSpacing(32);
    m_artwork = new QLabel;
    m_artwork->setObjectName("artwork");
    m_artwork->setMinimumSize(220, 220);
    m_artwork->setMaximumSize(300, 300);
    m_artwork->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_artwork->setScaledContents(true);
    content->addWidget(m_artwork, 0, Qt::AlignTop);

    auto *info = new QVBoxLayout;
    info->setSpacing(14);
    info->addStretch();
    m_title = new QLabel("No track selected");
    m_title->setObjectName("trackTitle");
    m_title->setWordWrap(true);
    m_artist = new QLabel("Artist unknown");
    m_artist->setObjectName("trackInfo");
    m_album = new QLabel("Album unknown");
    m_album->setObjectName("trackInfo");
    m_authors = new QLabel("Artist unknown");
    m_authors->setObjectName("trackInfoItalic");
    m_format = new QLabel("MP3     320 kbps     44.1 kHz     Stereo");
    m_format->setObjectName("trackMuted");
    info->addWidget(m_title);
    info->addWidget(m_artist);
    info->addWidget(m_album);
    info->addWidget(m_authors);
    info->addWidget(m_format);
    info->addStretch();
    content->addLayout(info, 1);
    layout->addLayout(content, 1);

    m_seek = new QSlider(Qt::Horizontal);
    m_seek->setRange(0, 0);
    m_seek->setToolTip(shortcutTip("Seek", "← / →"));
    connect(m_seek, &QSlider::sliderPressed, this, [this]{ m_seeking = true; });
    connect(m_seek, &QSlider::sliderReleased, this, [this]{ m_seeking = false; m_player.setPosition(m_seek->value()); });
    layout->addWidget(m_seek);

    auto *timeRow = new QHBoxLayout;
    m_leftTime = new QLabel("0:00");
    m_rightTime = new QLabel("0:00");
    timeRow->addWidget(m_leftTime);
    timeRow->addStretch();
    timeRow->addWidget(m_rightTime);
    layout->addLayout(timeRow);
    return wrap;
}

QWidget *MainWindow::buildControlBar()
{
    auto *bar = new QWidget;
    bar->setObjectName("controlBar");
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(24, 14, 24, 12);
    layout->setSpacing(18);

    auto *volIcon = new QLabel("🔊");
    m_volume = new QSlider(Qt::Horizontal);
    m_volume->setRange(0, 100);
    m_volume->setFixedWidth(175);
    m_volume->setValue(m_player.volume());
    m_volume->setToolTip(shortcutTip("Volume", "↑ / ↓"));
    m_volumePercent = new QLabel("40%");
    m_volumePercent->setObjectName("volumeText");
    connect(m_volume, &QSlider::valueChanged, &m_player, &PlayerController::setVolume);
    layout->addWidget(volIcon);
    layout->addWidget(m_volume);
    layout->addWidget(m_volumePercent);
    layout->addStretch();

    m_shuffle = controlButton("⤨", "Shuffle", "H");
    auto *prev = controlButton("⏮", "Previous", "P");
    m_playPause = controlButton("▶", "Play / Pause", "Space", true);
    auto *next = controlButton("⏭", "Next", "N");
    m_repeat = controlButton("↻", "Repeat Off", "R");
    layout->addWidget(m_shuffle);
    layout->addWidget(prev);
    layout->addWidget(m_playPause);
    layout->addWidget(next);
    layout->addWidget(m_repeat);
    layout->addStretch();

    m_speed = new QComboBox;
    m_speed->addItems({"0.5x", "0.75x", "1.0x", "1.25x", "1.5x", "2.0x"});
    m_speed->setCurrentText("1.0x");
    m_speed->setFixedWidth(88);
    m_speed->setToolTip("Playback speed");
    connect(m_speed, &QComboBox::currentTextChanged, this, [this](const QString &s){ m_player.setPlaybackRate(s.left(s.size()-1).toDouble()); });
    m_mute = controlButton("🔇", "Mute", "M");
    layout->addWidget(m_speed);
    layout->addWidget(m_mute);

    connect(m_shuffle, &QPushButton::clicked, &m_player, &PlayerController::toggleShuffle);
    connect(prev, &QPushButton::clicked, this, &MainWindow::playPrevious);
    connect(m_playPause, &QPushButton::clicked, &m_player, &PlayerController::togglePlayPause);
    connect(next, &QPushButton::clicked, this, &MainWindow::playNext);
    connect(m_repeat, &QPushButton::clicked, &m_player, &PlayerController::cycleRepeat);
    connect(m_mute, &QPushButton::clicked, &m_player, &PlayerController::toggleMute);

    return bar;
}

QPushButton *MainWindow::controlButton(const QString &icon, const QString &title, const QString &shortcut, bool primary)
{
    auto *b = new QPushButton(icon + "\n" + title + "\n" + shortcut);
    b->setToolTip(shortcutTip(title, shortcut));
    b->setCursor(Qt::PointingHandCursor);
    b->setProperty("active", false);
    if (primary) {
        b->setObjectName("playButton");
        b->setFixedSize(88, 88);
    } else {
        b->setMinimumSize(86, 66);
        b->setMaximumHeight(72);
    }
    return b;
}

void MainWindow::buildShortcuts()
{
    auto addShortcut = [this](const QKeySequence &sequence, auto slot) {
        auto *shortcut = new QShortcut(sequence, this);
        shortcut->setContext(Qt::ApplicationShortcut);
        connect(shortcut, &QShortcut::activated, this, slot);
    };

    addShortcut(QKeySequence(Qt::Key_Space), [this]{ m_player.togglePlayPause(); });
    addShortcut(QKeySequence::Open, [this]{ openFiles(); });
    addShortcut(QKeySequence("N"), [this]{ playNext(); });
    addShortcut(QKeySequence("P"), [this]{ playPrevious(); });
    addShortcut(QKeySequence(Qt::Key_Up), [this]{ m_player.volumeUp(); });
    addShortcut(QKeySequence(Qt::Key_Down), [this]{ m_player.volumeDown(); });
    addShortcut(QKeySequence(Qt::Key_Right), [this]{ m_player.seekBy(10000); });
    addShortcut(QKeySequence(Qt::Key_Left), [this]{ m_player.seekBy(-10000); });
    addShortcut(QKeySequence("M"), [this]{ m_player.toggleMute(); });
    addShortcut(QKeySequence("H"), [this]{ m_player.toggleShuffle(); });
    addShortcut(QKeySequence("R"), [this]{ m_player.cycleRepeat(); });
    addShortcut(QKeySequence::Quit, []{ qApp->quit(); });
    addShortcut(QKeySequence::Find, [this]{ m_search->setFocus(); });
    addShortcut(QKeySequence::Delete, [this]{ removeSelected(); });
}

void MainWindow::buildTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
    m_tray = new QSystemTrayIcon(QIcon(":/assets/assets/app_icon.png"), this);
    auto *menu = new QMenu(this);
    menu->addAction("Play / Pause", &m_player, &PlayerController::togglePlayPause);
    menu->addAction("Next Track", this, &MainWindow::playNext);
    menu->addAction("Previous Track", this, &MainWindow::playPrevious);
    menu->addSeparator();
    menu->addAction("Show SonicFlow", this, [this]{ show(); raise(); activateWindow(); });
    menu->addAction("Quit", qApp, &QApplication::quit);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip("SonicFlow Player");
    m_tray->show();
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet(R"(
        * { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial; font-size: 13px; color: #eaf4ff; }
        QMainWindow, QWidget { background: #07131d; }
        QMenuBar { background: transparent; padding: 6px 0; color: #d7e7f5; }
        QMenuBar::item { padding: 5px 12px; border-radius: 7px; }
        QMenuBar::item:selected { background: rgba(20,145,255,0.18); color: #28a8ff; }
        QMenu { background: #0b1824; border: 1px solid #263849; padding: 6px; }
        QMenu::item { padding: 8px 28px; border-radius: 5px; }
        QMenu::item:selected { background: #0d66d8; }
        QLabel#windowTitle { font-size: 17px; font-weight: 800; color: #f0f5fa; }
        QFrame#separator { color: rgba(255,255,255,0.09); background: rgba(255,255,255,0.09); max-height: 1px; }
        QWidget#toolbar { background: transparent; border-bottom: 1px solid rgba(255,255,255,0.09); }
        QToolButton { color: #e6f2ff; border: none; padding: 5px 8px; font-size: 14px; }
        QToolButton:hover { color: #20a7ff; background: rgba(255,255,255,0.04); border-radius: 7px; }
        QLineEdit { background: rgba(255,255,255,0.045); border: 1px solid rgba(255,255,255,0.08); border-radius: 7px; padding: 8px 12px; color: #f3f8ff; selection-background-color: #1477e8; }
        QLineEdit:focus { border: 1px solid #0f8cff; }
        QSplitter::handle { background: rgba(255,255,255,0.09); width: 1px; }
        QTableWidget { background: #0b1a26; border: none; color: #dbe9f8; gridline-color: rgba(255,255,255,0.06); selection-background-color: #0e72df; selection-color: #ffffff; outline: none; }
        QTableWidget::item { padding-left: 8px; border-bottom: 1px solid rgba(255,255,255,0.055); }
        QTableWidget::item:selected { background: #0f71db; color: white; }
        QHeaderView::section { background: #0c1c2a; color: #cbd9e8; border: none; border-bottom: 1px solid rgba(255,255,255,0.08); padding: 10px 8px; font-weight: 700; }
        QWidget#nowPanel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0b1a26, stop:1 #07131d); border-left: 1px solid rgba(255,255,255,0.10); }
        QLabel#tabActive { color: #ffffff; font-weight: 800; padding: 9px 44px; border-bottom: 3px solid #0e97ff; }
        QLabel#tab { color: #b5c6d4; padding: 9px 44px; }
        QLabel#artwork { border-radius: 4px; background: #102334; }
        QLabel#trackTitle { font-size: 20px; font-weight: 800; color: white; }
        QLabel#trackInfo { font-size: 14px; color: #e0edf9; }
        QLabel#trackInfoItalic { font-size: 14px; color: #dce8f4; font-style: italic; }
        QLabel#trackMuted { color: #9cafc1; font-size: 12px; }
        QSlider::groove:horizontal { height: 6px; border-radius: 3px; background: rgba(255,255,255,0.13); }
        QSlider::sub-page:horizontal { background: #139eff; border-radius: 3px; }
        QSlider::handle:horizontal { background: #eaf7ff; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }
        QWidget#controlBar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #102231, stop:1 #0b1722); border-top: 1px solid rgba(255,255,255,0.10); }
        QPushButton { background: transparent; border: none; color: #e8f4ff; padding: 3px 7px; font-size: 13px; }
        QPushButton:hover { color: #25aaff; }
        QPushButton[active="true"] { color: #25aaff; font-weight: 700; }
        QPushButton#playButton { background: #0c7df2; border-radius: 44px; font-size: 14px; font-weight: 800; }
        QLabel#volumeText { color: #dcecff; min-width: 42px; }
        QComboBox { background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.13); border-radius: 7px; padding: 8px 10px; }
        QStatusBar { background: #09131d; border-top: 1px solid rgba(255,255,255,0.08); color: #b4c5d5; }
    )");
}

void MainWindow::openFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Open Audio Files", QString(), "Audio Files (*.mp3 *.wav *.flac *.ogg *.aac *.m4a *.wma *.opus *.aiff);;All Files (*)");
    addFilesInternal(files, true, true);
}

void MainWindow::openFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (!folder.isEmpty()) addFilesInternal(audioFilesFromFolder(folder), true, true);
}

void MainWindow::addFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Add Audio Files", QString(), "Audio Files (*.mp3 *.wav *.flac *.ogg *.aac *.m4a *.wma *.opus *.aiff);;All Files (*)");
    addFilesInternal(files, false, m_playlist.count() == 0);
}

void MainWindow::savePlaylist()
{
    QString file = QFileDialog::getSaveFileName(this, "Save Playlist", "playlist.m3u", "Playlist (*.m3u *.txt)");
    if (!file.isEmpty() && !m_playlist.saveToFile(file)) QMessageBox::warning(this, "Save Playlist", "Could not save playlist.");
}

void MainWindow::loadPlaylist()
{
    QString file = QFileDialog::getOpenFileName(this, "Load Playlist", QString(), "Playlist (*.m3u *.txt)");
    if (!file.isEmpty()) { m_playlist.loadFromFile(file); if (m_playlist.count()) playIndex(0); }
}

void MainWindow::removeSelected()
{
    int row = selectedRow();
    if (row >= 0) m_playlist.removeRows({row});
}

void MainWindow::clearPlaylist() { m_player.stop(); m_playlist.clear(); }
void MainWindow::removeDuplicates() { m_playlist.removeDuplicates(); }
void MainWindow::playSelectedRow() { int row = selectedRow(); if (row >= 0) playIndex(row); }

void MainWindow::playIndex(int index)
{
    if (index < 0 || index >= m_playlist.count()) return;
    m_playlist.setCurrentIndex(index);
    m_player.playUrl(m_playlist.urlAt(index));
    m_table->selectRow(index);
}

void MainWindow::playNext()
{
    if (!m_playlist.count()) return;
    int next = 0;
    if (m_player.shuffle()) next = QRandomGenerator::global()->bounded(m_playlist.count());
    else next = (m_playlist.currentIndex() + 1) % m_playlist.count();
    playIndex(next);
}

void MainWindow::playPrevious()
{
    if (!m_playlist.count()) return;
    int prev = m_playlist.currentIndex() - 1;
    if (prev < 0) prev = m_playlist.count() - 1;
    playIndex(prev);
}

void MainWindow::updatePlaylist()
{
    m_table->setRowCount(m_playlist.count());
    for (int i = 0; i < m_playlist.count(); ++i) {
        const auto t = m_playlist.track(i);
        auto mk = [](const QString &s){ auto *it = new QTableWidgetItem(s); it->setToolTip(s); return it; };
        m_table->setItem(i, 0, mk(QString::number(i + 1)));
        m_table->setItem(i, 1, mk(t.title));
        m_table->setItem(i, 2, mk(t.artist));
        m_table->setItem(i, 3, mk(t.album));
        m_table->setItem(i, 4, mk(formatTime(t.duration)));
        m_table->setRowHeight(i, 46);
    }
    m_statusLeft->setText(QString("%1 tracks, %2 total duration").arg(m_playlist.count()).arg(formatTime(m_playlist.totalDuration())));
    filterRows(m_search->text());
}

void MainWindow::updateNowPlaying()
{
    int idx = m_playlist.currentIndex();
    if (idx < 0 || idx >= m_playlist.count()) {
        setArtwork(m_metadata.defaultArtwork(280));
        m_title->setText("No track selected");
        m_artist->setText("Artist unknown");
        m_album->setText("Album unknown");
        m_authors->setText("Artist unknown");
        m_statusCenter->setText("Ready");
        return;
    }
    auto t = m_playlist.track(idx);
    m_metadata.applyMetadata(t, m_player.player()->metaData());
    setSongInfo(t);
    setArtwork(m_metadata.artworkFromMeta(m_player.player()->metaData(), 280));
    m_statusCenter->setText("Playing: " + t.title);
    m_table->selectRow(idx);
    if (m_tray && m_tray->supportsMessages()) m_tray->showMessage("Now Playing", t.title + " — " + t.artist, QSystemTrayIcon::Information, 2500);
}

void MainWindow::updatePlaybackState(QMediaPlayer::PlaybackState state)
{
    m_playPause->setText((state == QMediaPlayer::PlayingState ? "⏸\nPause\nSpace" : "▶\nPlay\nSpace"));
}

void MainWindow::updatePosition(qint64 pos)
{
    if (!m_seeking) m_seek->setValue(static_cast<int>(pos));
    m_leftTime->setText(formatTime(pos));
}

void MainWindow::updateDuration(qint64 duration)
{
    m_duration = duration;
    m_seek->setRange(0, static_cast<int>(duration));
    m_rightTime->setText(formatTime(duration));
}

void MainWindow::handleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        if (m_player.repeatMode() == PlayerController::RepeatOne) playIndex(m_playlist.currentIndex());
        else if (m_player.repeatMode() == PlayerController::RepeatAll || m_playlist.currentIndex() + 1 < m_playlist.count()) playNext();
    }
}

void MainWindow::updateVolumeLabel(int volume)
{
    if (m_volume && m_volume->value() != volume) m_volume->setValue(volume);
    m_volumePercent->setText(QString::number(volume) + "%");
}

void MainWindow::updateMuteState(bool muted)
{
    if (!m_mute) return;
    m_mute->setText(muted ? "🔇\nMuted\nM" : "🔊\nMute\nM");
    m_mute->setProperty("active", muted);
    m_mute->style()->unpolish(m_mute); m_mute->style()->polish(m_mute);
}

void MainWindow::updateShuffleState(bool enabled)
{
    if (!m_shuffle) return;
    m_shuffle->setText(enabled ? "⤨\nShuffle On\nH" : "⤨\nShuffle\nH");
    m_shuffle->setProperty("active", enabled);
    m_shuffle->style()->unpolish(m_shuffle); m_shuffle->style()->polish(m_shuffle);
}

QString MainWindow::repeatLabel() const
{
    if (m_player.repeatMode() == PlayerController::RepeatOne) return "Repeat One";
    if (m_player.repeatMode() == PlayerController::RepeatAll) return "Repeat All";
    return "Repeat Off";
}

void MainWindow::updateRepeatState(PlayerController::RepeatMode mode)
{
    Q_UNUSED(mode);
    if (!m_repeat) return;
    m_repeat->setText("↻\n" + repeatLabel() + "\nR");
    m_repeat->setProperty("active", m_player.repeatMode() != PlayerController::RepeatOff);
    m_repeat->style()->unpolish(m_repeat); m_repeat->style()->polish(m_repeat);
}

void MainWindow::showHelp()
{
    QMessageBox::information(this, "Keyboard Shortcuts",
        "Play / Pause          Space\n"
        "Open File             Ctrl+O / Cmd+O\n"
        "Next Track            N\n"
        "Previous Track        P\n"
        "Volume Up             ↑\n"
        "Volume Down           ↓\n"
        "Seek Forward          →\n"
        "Seek Back             ←\n"
        "Mute                  M\n"
        "Shuffle               H\n"
        "Repeat                R\n"
        "Lyrics                Click Lyrics tab\n"
        "Quit                  Ctrl+Q / Cmd+Q");
}

void MainWindow::showLyrics()
{
    QString text = "No lyrics found for this track.\n\nTip: put a .lrc or .txt lyrics file with the same name as the audio file in the same folder.";
    const int idx = m_playlist.currentIndex();
    if (idx >= 0 && idx < m_playlist.count()) {
        const auto track = m_playlist.track(idx);
        const QFileInfo info(track.path);
        const QString base = info.absolutePath() + "/" + info.completeBaseName();
        const QStringList candidates { base + ".lrc", base + ".txt" };
        for (const QString &candidate : candidates) {
            QFile f(candidate);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                text = QString::fromUtf8(f.readAll()).trimmed();
                if (text.isEmpty()) text = "Lyrics file is empty.";
                break;
            }
        }
    }
    QMessageBox box(this);
    box.setWindowTitle("Lyrics");
    box.setTextFormat(Qt::PlainText);
    box.setText(text.left(6000));
    box.exec();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About SonicFlow Player", "SonicFlow Player\nModern Qt 6 audio player using Qt Multimedia.\nNo VLC or third-party audio backend.");
}

void MainWindow::filterRows(const QString &text)
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        bool match = text.trimmed().isEmpty();
        for (int c = 1; c <= 3 && !match; ++c) match = m_table->item(r,c)->text().contains(text, Qt::CaseInsensitive);
        m_table->setRowHidden(r, !match);
    }
}

void MainWindow::addFilesInternal(const QStringList &files, bool replace, bool autoPlay)
{
    if (files.isEmpty()) return;
    m_playlist.addFiles(files, replace);
    if (autoPlay && m_playlist.count()) playIndex(replace ? 0 : m_playlist.count() - files.size());
}

QStringList MainWindow::audioFilesFromFolder(const QString &folder) const
{
    QStringList out;
    QDirIterator it(folder, {"*.mp3","*.wav","*.flac","*.ogg","*.aac","*.m4a","*.wma","*.opus","*.aiff"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) out << it.next();
    return out;
}

QString MainWindow::formatTime(qint64 ms) const
{
    if (ms <= 0) return "0:00";
    qint64 s = ms / 1000;
    qint64 h = s / 3600; s %= 3600;
    qint64 m = s / 60; s %= 60;
    return h ? QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')) : QString("%1:%2").arg(m).arg(s,2,10,QChar('0'));
}

QString MainWindow::shortcutTip(const QString &name, const QString &keys) const { return name + "  (" + keys + ")"; }
void MainWindow::saveSettings() { m_settings.setValue("geometry", saveGeometry()); m_settings.setValue("volume", m_player.volume()); m_settings.setValue("splitter", m_splitter->saveState()); }
void MainWindow::loadSettings() { restoreGeometry(m_settings.value("geometry").toByteArray()); m_player.setVolume(m_settings.value("volume", 40).toInt()); m_splitter->restoreState(m_settings.value("splitter").toByteArray()); }
void MainWindow::setArtwork(const QPixmap &pixmap) { m_artwork->setPixmap(pixmap.scaled(m_artwork->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)); }
void MainWindow::setSongInfo(const TrackItem &track) { m_title->setText(track.title); m_artist->setText("▧  " + track.artist); m_album->setText("▧  " + track.album); m_authors->setText("♙  " + track.artist); }
int MainWindow::selectedRow() const { auto rows = m_table->selectionModel()->selectedRows(); return rows.isEmpty() ? -1 : rows.first().row(); }


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();
    const auto mods = event->modifiers();
    const bool cmdOrCtrl = mods.testFlag(Qt::ControlModifier) || mods.testFlag(Qt::MetaModifier);
    if (qobject_cast<QLineEdit *>(QApplication::focusWidget()) && !cmdOrCtrl) {
        QMainWindow::keyPressEvent(event);
        return;
    }
    if (cmdOrCtrl && key == Qt::Key_O) { openFiles(); event->accept(); return; }
    if (cmdOrCtrl && key == Qt::Key_Q) { qApp->quit(); event->accept(); return; }
    switch (key) {
    case Qt::Key_Space: m_player.togglePlayPause(); event->accept(); return;
    case Qt::Key_N: playNext(); event->accept(); return;
    case Qt::Key_P: playPrevious(); event->accept(); return;
    case Qt::Key_Up: m_player.volumeUp(); event->accept(); return;
    case Qt::Key_Down: m_player.volumeDown(); event->accept(); return;
    case Qt::Key_Right: m_player.seekBy(10000); event->accept(); return;
    case Qt::Key_Left: m_player.seekBy(-10000); event->accept(); return;
    case Qt::Key_M: m_player.toggleMute(); event->accept(); return;
    case Qt::Key_H: m_player.toggleShuffle(); event->accept(); return;
    case Qt::Key_R: m_player.cycleRepeat(); event->accept(); return;
    default: QMainWindow::keyPressEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride)
        return QMainWindow::eventFilter(watched, event);

    if (QApplication::activeWindow() != this && !isAncestorOf(QApplication::focusWidget()))
        return QMainWindow::eventFilter(watched, event);

    auto *keyEvent = static_cast<QKeyEvent *>(event);
    const int key = keyEvent->key();
    const auto mods = keyEvent->modifiers();
    const bool cmdOrCtrl = mods.testFlag(Qt::ControlModifier) || mods.testFlag(Qt::MetaModifier);

    if (qobject_cast<QLineEdit *>(QApplication::focusWidget()) && !cmdOrCtrl)
        return QMainWindow::eventFilter(watched, event);

    bool matched = false;
    if (cmdOrCtrl && (key == Qt::Key_O || key == Qt::Key_Q)) matched = true;
    if (key == Qt::Key_Space || key == Qt::Key_N || key == Qt::Key_P || key == Qt::Key_Up || key == Qt::Key_Down ||
        key == Qt::Key_Right || key == Qt::Key_Left || key == Qt::Key_M || key == Qt::Key_H || key == Qt::Key_R) matched = true;
    if (!matched) return QMainWindow::eventFilter(watched, event);

    keyEvent->accept();
    if (event->type() == QEvent::ShortcutOverride) return true;

    if (cmdOrCtrl && key == Qt::Key_O) { openFiles(); return true; }
    if (cmdOrCtrl && key == Qt::Key_Q) { qApp->quit(); return true; }
    switch (key) {
    case Qt::Key_Space: m_player.togglePlayPause(); return true;
    case Qt::Key_N: playNext(); return true;
    case Qt::Key_P: playPrevious(); return true;
    case Qt::Key_Up: m_player.volumeUp(); return true;
    case Qt::Key_Down: m_player.volumeDown(); return true;
    case Qt::Key_Right: m_player.seekBy(10000); return true;
    case Qt::Key_Left: m_player.seekBy(-10000); return true;
    case Qt::Key_M: m_player.toggleMute(); return true;
    case Qt::Key_H: m_player.toggleShuffle(); return true;
    case Qt::Key_R: m_player.cycleRepeat(); return true;
    default: break;
    }
    return QMainWindow::eventFilter(watched, event);
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event) { if (event->mimeData()->hasUrls()) event->acceptProposedAction(); }
void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    for (const QUrl &u : event->mimeData()->urls()) if (u.isLocalFile()) files << u.toLocalFile();
    addFilesInternal(files, false, m_playlist.count() == 0);
}
void MainWindow::closeEvent(QCloseEvent *event) { saveSettings(); QMainWindow::closeEvent(event); }
