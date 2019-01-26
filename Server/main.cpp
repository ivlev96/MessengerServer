#include "data/databasecontroller.h"

#include <QTextStream>

using namespace Data;

int main(int argc, char *argv[])
{
	DatabaseController controller;

	const QString lastError = controller.lastError();
	if (!lastError.isEmpty())
	{
		QTextStream(stdout) << lastError;
		system("pause");
	}
	return 0;
}
