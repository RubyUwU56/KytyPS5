#include "mainDialog.h"
#include "launcherTheme.h"
#include "consoleHome.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	// Title bars and taskbars get the bolder small-size artwork.
	const QIcon small_art(QStringLiteral(":/icons/kyty-small.svg"));
	const QIcon large_art(QStringLiteral(":/icons/kyty.svg"));
	QIcon       icon;
	for (int size: {16, 24, 32, 48, 64}) {
		icon.addPixmap(small_art.pixmap(QSize(size, size), 1.0));
	}
	for (int size: {128, 256}) {
		icon.addPixmap(large_art.pixmap(QSize(size, size), 1.0));
	}
	QApplication::setWindowIcon(icon);
	LauncherTheme::Initialize(a);

	MainDialog w;
	if (a.arguments().contains(QStringLiteral("--desktop"))) {
		w.emit Start();
		w.show();
		return QApplication::exec();
	}
	ConsoleHome home(&w);
	if (a.arguments().contains(QStringLiteral("--windowed"))) home.show();
	else home.showFullScreen();

	return QApplication::exec();
}
