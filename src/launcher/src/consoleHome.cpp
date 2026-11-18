#include "consoleHome.h"

#include "configurationItem.h"
#include "configurationListWidget.h"
#include "controllerPreview.h"
#include "gameContent.h"
#include "gameViews.h"
#include "mainDialog.h"

#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QStyledItemDelegate>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
class ConsoleCoverDelegate final: public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override { return {164, 196}; }
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		const bool selected = option.state.testFlag(QStyle::State_Selected);
		const QRect cover(option.rect.x() + 8, option.rect.y() + 8, 148, 148);
		painter->setPen(selected ? QPen(QColor(245, 226, 185), 3) : QPen(QColor(255, 255, 255, 45), 1));
		painter->setBrush(QColor(19, 28, 43, 220));
		painter->drawRoundedRect(cover.adjusted(-3, -3, 3, 3), 12, 12);
		index.data(Qt::DecorationRole).value<QIcon>().paint(painter, cover.adjusted(3, 3, -3, -3));
		painter->setFont(option.font);
		painter->setPen(QColor(240, 242, 248));
		painter->drawText(QRect(cover.x(), cover.bottom() + 12, cover.width(), 24), Qt::AlignLeft | Qt::AlignVCenter,
		                  option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, cover.width()));
		painter->restore();
	}
};
}

ConsoleHome::ConsoleHome(MainDialog* launcher)
    : m_launcher(launcher), m_library(launcher->Library()), m_controller(launcher->Controller()),
      m_tree(m_library->GameList()) {
	setObjectName(QStringLiteral("ConsoleHome"));
	setWindowTitle(QStringLiteral("KytyPS5 Home"));
	setMinimumSize(960, 600);
	resize(1280, 720);
	m_launcher->SetConsoleMode(true);
	setStyleSheet(QStringLiteral(R"(
        QWidget#ConsoleHome { color: #f2f4fa; font-family: 'Segoe UI'; font-size: 16px; }
        QWidget#ConsoleHome QLabel { color: #f2f4fa; background: transparent; }
        QWidget#ConsoleHome QPushButton { color: #e6e9f2; background: rgba(30,39,57,160);
            border: 1px solid rgba(255,255,255,35); border-radius: 20px; padding: 10px 24px; }
        QWidget#ConsoleHome QPushButton:hover, QWidget#ConsoleHome QPushButton:focus {
            border: 2px solid #f1d8a0; background: rgba(60,74,100,210); }
        QWidget#ConsoleHome QPushButton#Play { color: #101724; background: #eff3fb;
            border-radius: 25px; padding: 13px 48px; font-weight: 600; }
        QWidget#ConsoleHome QPushButton#Play:disabled { background: #576176; color: #c3c9d4; }
        QWidget#ConsoleHome QListView { background: transparent; border: none; outline: none; }
    )"));
	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	m_pages = new QStackedWidget(this);
	root->addWidget(m_pages);
	auto* boot = new QWidget;
	boot->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_pages->addWidget(boot);
	auto* home = new QWidget;
	m_pages->addWidget(home);
	auto* layout = new QVBoxLayout(home);
	layout->setContentsMargins(64, 36, 64, 32);
	layout->setSpacing(22);
	auto* top = new QHBoxLayout;
	auto* brand = new QLabel(QStringLiteral("KYTY  /  HOME"));
	brand->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600; color: #c5ccd9;"));
	top->addWidget(brand);
	top->addSpacing(20);
	const auto button = [top, this](const QString& text, auto callback) {
		auto* control = new QPushButton(text);
		top->addWidget(control);
		connect(control, &QPushButton::clicked, this, callback);
		return control;
	};
	button(tr("Games"), [this] { SetLibraryMode(false); });
	button(tr("Library"), [this] { SetLibraryMode(true); });
	top->addStretch();
	button(tr("Settings"), [this] { m_library->OpenSettings(); UpdateSelection(); });
	button(tr("Desktop"), [this] { OpenDesktop(); });
	button(tr("Exit"), [this] { close(); });
	m_clock = new QLabel;
	top->addWidget(m_clock);
	layout->addLayout(top);
	m_games = new QListView;
	SetupGameGrid(m_tree, m_games);
	m_games->setItemDelegate(new ConsoleCoverDelegate(m_games));
	m_games->setIconSize(QSize(142, 142));
	m_games->setSpacing(12);
	m_games->setWrapping(false);
	m_games->setMinimumHeight(220);
	m_games->setMaximumHeight(230);
	m_games->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(m_games);
	layout->addStretch(1);
	m_title = new QLabel;
	m_title->setWordWrap(true);
	m_title->setStyleSheet(QStringLiteral("font-size: 44px; font-weight: 300;"));
	m_title->setTextFormat(Qt::PlainText);
	layout->addWidget(m_title);
	m_details = new QLabel;
	m_details->setStyleSheet(QStringLiteral("color: #c2cad8; font-size: 16px;"));
	m_details->setTextFormat(Qt::PlainText);
	layout->addWidget(m_details);
	auto* actions = new QHBoxLayout;
	m_play = new QPushButton(tr("Play Game"));
	m_play->setObjectName(QStringLiteral("Play"));
	actions->addWidget(m_play);
	auto* game_settings = new QPushButton(tr("Game Settings"));
	actions->addWidget(game_settings);
	actions->addStretch();
	layout->addLayout(actions);
	layout->addSpacing(14);
	auto* hints = new QLabel(tr("D-pad  Navigate     Enter / Cross  Select     Esc / Circle  Back     Tab / R1  Next control     F11  Fullscreen"));
	hints->setWordWrap(true);
	hints->setStyleSheet(QStringLiteral("color: #9ca9c1; font-size: 13px;"));
	layout->addWidget(hints);
	connect(m_play, &QPushButton::clicked, m_library, &ConfigurationListWidget::LaunchSelected);
	connect(game_settings, &QPushButton::clicked, m_library, &ConfigurationListWidget::OpenGameSettings);
	connect(m_games, &QListView::doubleClicked, this, [this] { m_library->LaunchSelected(); });
	connect(m_library, &ConfigurationListWidget::Select, this, [this] { UpdateSelection(); });
	const auto ensure_selection = [this] {
		QTimer::singleShot(0, this, [this] {
			if (!m_tree->currentItem() && m_tree->topLevelItemCount() > 0)
				m_tree->setCurrentItem(m_tree->topLevelItem(0));
			UpdateSelection();
		});
	};
	connect(m_tree->model(), &QAbstractItemModel::modelReset, this, ensure_selection);
	connect(m_tree->model(), &QAbstractItemModel::rowsInserted, this, ensure_selection);
	connect(m_tree->model(), &QAbstractItemModel::rowsRemoved, this, ensure_selection);
	connect(m_launcher, &MainDialog::GameRunningChanged, this, [this](bool running) {
		m_running = running;
		if (running) {
			m_frames->stop();
			m_launcher->hide();
			hide();
		} else {
			show();
			raise();
			activateWindow();
			m_frames->start();
			m_games->setFocus();
		}
		UpdateSelection();
	});
	m_launcher->installEventFilter(this);
	qApp->installEventFilter(this);
	m_time.start();
	m_repeat.start();
	m_frames = new QTimer(this);
	m_frames->setInterval(33);
	connect(m_frames, &QTimer::timeout, this, [this] {
		m_clock->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
		PollController();
		if (isVisible()) update();
	});
	m_frames->start();
	m_controller->SetPreviewEnabled(true);
	UpdateSelection();
	QTimer::singleShot(3200, this, [this] { EnterHome(); });
}

ConsoleHome::~ConsoleHome() {
	qApp->removeEventFilter(this);
	m_launcher->removeEventFilter(this);
	m_controller->SetPreviewEnabled(false);
}

void ConsoleHome::EnterHome() {
	if (!m_booting) return;
	m_booting = false;
	m_pages->setCurrentIndex(1);
	emit m_launcher->Start();
	if (!m_tree->currentItem() && m_tree->topLevelItemCount() > 0)
		m_tree->setCurrentItem(m_tree->topLevelItem(0));
	m_games->setFocus();
	UpdateSelection();
}

void ConsoleHome::UpdateSelection() {
	const auto* item = m_library->GetSelectedItem();
	m_play->setEnabled(m_library->CanLaunch());
	m_title->setText(item ? item->GetInfo().name : tr("Your next world awaits"));
	m_details->setText(item ? QStringLiteral("%1  /  %2").arg(item->GetInfo().title_id, item->GetInfo().gameVersion)
	                        : tr("Open Settings to add your game folders, then explore your library."));
	const auto path = item ? item->GetInfo().basedir : QString();
	if (path != m_background_path) {
		m_background_path = path;
		m_background = {};
		if (!path.isEmpty())
			m_background.loadFromData(GameContent::ReadFile(path, QStringLiteral("sce_sys/pic0.png"), GameContent::MaxImageSize));
		update();
	}
}

void ConsoleHome::SetLibraryMode(bool library) {
	m_games->setWrapping(library);
	m_games->setMaximumHeight(library ? QWIDGETSIZE_MAX : 230);
	m_games->setVerticalScrollBarPolicy(library ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	m_games->setFocus();
}

void ConsoleHome::OpenDesktop() {
	m_launcher->show();
	m_launcher->raise();
	m_launcher->activateWindow();
}

void ConsoleHome::SendKey(int key) {
	auto* target = QApplication::focusWidget();
	if (!target) target = this;
	QKeyEvent down(QEvent::KeyPress, key, Qt::NoModifier);
	QKeyEvent up(QEvent::KeyRelease, key, Qt::NoModifier);
	QApplication::sendEvent(target, &down);
	QApplication::sendEvent(target, &up);
}

void ConsoleHome::PollController() {
	if (m_running) return;
	const auto pressed = m_controller->Pressed();
	if (QApplication::applicationState() != Qt::ApplicationActive) {
		m_pressed = pressed;
		m_last_direction = 0;
		return;
	}
	auto* active = QApplication::activeWindow();
	if (active != this && active != m_launcher && !QApplication::activeModalWidget()) {
		m_pressed = pressed;
		return;
	}
	int direction = 0;
	if (pressed.contains(QStringLiteral("Left")) || pressed.contains(QStringLiteral("LeftStickLeft"))) direction = Qt::Key_Left;
	else if (pressed.contains(QStringLiteral("Right")) || pressed.contains(QStringLiteral("LeftStickRight"))) direction = Qt::Key_Right;
	else if (pressed.contains(QStringLiteral("Up")) || pressed.contains(QStringLiteral("LeftStickUp"))) direction = Qt::Key_Up;
	else if (pressed.contains(QStringLiteral("Down")) || pressed.contains(QStringLiteral("LeftStickDown"))) direction = Qt::Key_Down;
	if (direction && (direction != m_last_direction || m_repeat.elapsed() >= 180)) {
		SendKey(direction);
		m_repeat.restart();
	}
	m_last_direction = direction;
	for (const auto& binding: {std::pair {"Cross", Qt::Key_Return}, {"Circle", Qt::Key_Escape},
	                           {"L1", Qt::Key_Backtab}, {"R1", Qt::Key_Tab}}) {
		const auto name = QString::fromLatin1(binding.first);
		if (pressed.contains(name) && !m_pressed.contains(name)) SendKey(binding.second);
	}
	m_pressed = pressed;
}

bool ConsoleHome::eventFilter(QObject* watched, QEvent* event) {
	if (watched == this && event->type() == QEvent::WindowActivate && !m_running)
		m_controller->SetPreviewEnabled(true);
	if (watched == m_launcher && event->type() == QEvent::Close) {
		m_launcher->hide();
		event->ignore();
		show();
		activateWindow();
		return true;
	}
	if (event->type() == QEvent::KeyPress && isActiveWindow() && !QApplication::activeModalWidget()) {
		const auto key = static_cast<QKeyEvent*>(event)->key();
		if (m_booting && (key == Qt::Key_Return || key == Qt::Key_Space || key == Qt::Key_Escape)) {
			EnterHome();
			return true;
		}
		if (key == Qt::Key_F11) {
			isFullScreen() ? showNormal() : showFullScreen();
			return true;
		}
		if (key == Qt::Key_Escape) { SetLibraryMode(false); return true; }
		if ((key == Qt::Key_Return || key == Qt::Key_Enter) && m_games->hasFocus()) {
			m_library->LaunchSelected();
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void ConsoleHome::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	const double seconds = m_time.elapsed() / 1000.0;
	QLinearGradient base(0, 0, width(), height());
	base.setColorAt(0, QColor(6, 12, 23));
	base.setColorAt(0.55, QColor(16, 30, 48));
	base.setColorAt(1, QColor(4, 8, 17));
	painter.fillRect(rect(), base);
	if (!m_booting && !m_background.isNull()) {
		const auto scaled = m_background.size().scaled(size(), Qt::KeepAspectRatioByExpanding);
		const QRect target((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled.width(), scaled.height());
		painter.drawPixmap(target, m_background);
		QLinearGradient shade(0, 0, width(), height());
		shade.setColorAt(0, QColor(3, 7, 16, 175));
		shade.setColorAt(0.5, QColor(3, 7, 16, 70));
		shade.setColorAt(1, QColor(3, 7, 16, 235));
		painter.fillRect(rect(), shade);
	}
	// Original procedural light ribbons and dust; no firmware or video assets.
	for (int band = 0; band < 6; ++band) {
		QPainterPath wave;
		for (int x = 0; x <= width() + 16; x += 16) {
			const double y = height() * (0.54 + 0.018 * band) +
			                 std::sin(x / double(std::max(1, width())) * 5 + seconds * 0.15 + band * 0.18) * height() * 0.13;
			if (x == 0) wave.moveTo(x, y); else wave.lineTo(x, y);
		}
		painter.setPen(QPen(QColor(208, 185, 130, m_booting ? 24 : 9), 1.5));
		painter.drawPath(wave);
	}
	painter.setPen(Qt::NoPen);
	for (int index = 0; index < 65; ++index) {
		const double x = std::fmod(index * 0.61803398875 + seconds * (0.002 + index % 4 * 0.0003), 1.0) * width();
		const double y = std::fmod(index * 0.38196601125 + 1.0 - std::fmod(seconds * 0.006, 1.0), 1.0) * height();
		painter.setBrush(QColor(239, 215, 164, m_booting ? 70 + index % 90 : 25));
		painter.drawEllipse(QPointF(x, y), 0.8 + index % 3 * 0.3, 0.8 + index % 3 * 0.3);
	}
	if (m_booting) {
		painter.setOpacity(std::clamp(seconds / 0.8, 0.0, 1.0));
		auto font = painter.font();
		font.setPixelSize(std::clamp(width() / 16, 46, 110));
		font.setWeight(QFont::Light);
		font.setLetterSpacing(QFont::AbsoluteSpacing, 12);
		painter.setFont(font);
		painter.setPen(QColor(242, 243, 247));
		painter.drawText(rect().adjusted(0, -30, 0, 0), Qt::AlignCenter, QStringLiteral("KYTY"));
		font.setPixelSize(15);
		font.setLetterSpacing(QFont::AbsoluteSpacing, 4);
		painter.setFont(font);
		painter.setPen(QColor(190, 201, 219));
		painter.drawText(rect().adjusted(0, 130, 0, 0), Qt::AlignCenter, tr("WELCOME HOME"));
		font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
		font.setPixelSize(13);
		painter.setFont(font);
		painter.drawText(rect().adjusted(0, 0, 0, -45), Qt::AlignHCenter | Qt::AlignBottom, tr("Press Enter or Cross to continue"));
	}
}
