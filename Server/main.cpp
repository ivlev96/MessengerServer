#include "controllers/globalcontroller.h"

#include <QTextStream>

using namespace Controllers;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	GlobalController controller;
	return a.exec();
}
