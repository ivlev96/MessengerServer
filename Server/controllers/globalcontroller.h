#pragma once

#include "data/databasecontroller.h"
#include "network/servercontroller.h"

namespace Controllers
{

class GlobalController : public QObject
{
	Q_OBJECT

public:
	GlobalController(QObject *parent = nullptr);
	~GlobalController();

private slots:
	void onError(const QString& error);

private:
	QTextStream m_cout;
	std::unique_ptr<Data::DatabaseController> m_db;
	Network::ServerController* m_server;
	QThread* m_serverThread;
};

}
