#ifndef MAIN_DIALOG_H
#define MAIN_DIALOG_H

#include <QDialog>
#include <QString>

class QWidget;
class QProcess;
class MainDialogPrivate;
class QSettings;
class QResizeEvent;

class Configuration;
class MainDialog: public QDialog {
	Q_OBJECT

signals:
	void Start();
	void Resize();
	void GameRunningChanged(bool running);

public:
	explicit MainDialog(QWidget* parent = nullptr);
	~MainDialog() override = default;

	void RunInterpreter(QProcess* process, const Configuration& info, bool close_terminal);
	class ConfigurationListWidget* Library() const;
	class ControllerPreview* Controller() const;
	void SetConsoleMode(bool enabled) { m_console_mode = enabled; }

	static void WriteSettings(QSettings& s);
	static void ReadSettings(QSettings& s);

	void resizeEvent(QResizeEvent* event) override;

private:
	MainDialogPrivate* m_p = nullptr;
	bool m_console_mode = false;
};

#endif // MAIN_DIALOG_H
