/*
 * \brief   Tuna board hardware control app
 * \author  Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date    2013-02-15
 */

/* Qt includes */
#include <QtGui>
#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
	static QApplication app(argc, argv);
	QLabel l;
	l.setText("Hello World");
	l.resize(400, 200);
	l.show();

	return app.exec();
}
