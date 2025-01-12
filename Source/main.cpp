// Created by Eternity_boundary on Jan 4,2025
#include "Headers\Celestian.h"
#include <QApplication>
#include <Qfile>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	QFile file(":/Celestian/dark_theme.qss");
	if (file.open(QFile::ReadOnly)) {
		QString styleSheet = QLatin1String(file.readAll());
		qApp->setStyleSheet(styleSheet);
		file.close();
	}

	Celestian w;
	w.setWindowTitle("Celestian++");
	w.show();
	return a.exec();
}