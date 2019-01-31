#include "globalcontroller.h"

using namespace Controllers;
using namespace Network;
using namespace Data;

GlobalController::GlobalController(QObject *parent)
	: QObject(parent)
	, m_db(std::make_shared<DatabaseController>())
	, m_server(new ServerController(Common::serverPort))
	, m_serverThread(new QThread(this))
{
	m_server->moveToThread(m_serverThread);
	assert(connect(m_serverThread, &QThread::finished, m_server, &QObject::deleteLater));

	m_serverThread->start();
	assert(m_serverThread->isRunning());
}

GlobalController::~GlobalController()
{
	m_serverThread->quit();
	m_serverThread->wait();
}
