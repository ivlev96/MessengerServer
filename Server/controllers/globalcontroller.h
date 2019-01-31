#pragma once

#include "data/databasecontroller.h"
#include "network/servercontroller.h"

#include <QObject>
#include <QThread>

namespace Controllers
{

class GlobalController : public QObject
{
	Q_OBJECT

public:
	GlobalController(QObject *parent = nullptr);
	~GlobalController();


private:
	std::shared_ptr<Data::DatabaseController> m_db;
	Network::ServerController* m_server;
	QThread* m_serverThread;
};

}
