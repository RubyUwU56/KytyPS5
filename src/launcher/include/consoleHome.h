#ifndef CONSOLE_HOME_H
#define CONSOLE_HOME_H

#include <QElapsedTimer>
#include <QPixmap>
#include <QSet>
#include <QWidget>

class MainDialog;
class ConfigurationListWidget;
class ControllerPreview;
class QListView;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTreeWidget;
class QTimer;

class ConsoleHome final: public QWidget {
public:
	explicit ConsoleHome(MainDialog* launcher);
	~ConsoleHome() override;

protected:
	void paintEvent(QPaintEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	void EnterHome();
	void UpdateSelection();
	void OpenDesktop();
	void PollController();
	void SendKey(int key);
	void SetLibraryMode(bool library);

	MainDialog* m_launcher;
	ConfigurationListWidget* m_library;
	ControllerPreview* m_controller;
	QTreeWidget* m_tree;
	QStackedWidget* m_pages;
	QListView* m_games;
	QLabel* m_title;
	QLabel* m_details;
	QLabel* m_clock;
	QPushButton* m_play;
	QTimer* m_frames;
	QElapsedTimer m_time;
	QElapsedTimer m_repeat;
	QSet<QString> m_pressed;
	QString m_background_path;
	QPixmap m_background;
	bool m_booting = true;
	bool m_running = false;
	int m_last_direction = 0;
};

#endif
