#include "CameraCalibration.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	CameraCalibration w;
	w.show();
	return a.exec();
}
