#include "globalcontroller.h"

using namespace Controllers;
using namespace Network;
using namespace Data;

GlobalController::GlobalController(QObject *parent)
	: QObject(parent)
	, m_db(std::make_unique<DatabaseController>())
	, m_server(new ServerController(Common::serverPort))
	, m_serverThread(new QThread(this))
	, m_cout(stdout)
{
	m_server->moveToThread(m_serverThread);
	assert(connect(m_serverThread, &QThread::finished, m_server, &QObject::deleteLater));
	assert(connect(m_serverThread, &QThread::started, m_server, &ServerController::onThreadStarted));

	assert(connect(m_server, &ServerController::processClientQuery, m_db.get(), &DatabaseController::processClientQuery));
	assert(connect(m_db.get(), &DatabaseController::responseReady, m_server, &ServerController::onResponseReady));

	//errors
	assert(connect(m_server, &ServerController::error, this, &GlobalController::onError));
	assert(connect(m_db.get(), &DatabaseController::error, this, &GlobalController::onError));

	m_serverThread->start();
	assert(m_serverThread->isRunning());
}

GlobalController::~GlobalController()
{
	m_serverThread->quit();
	m_serverThread->wait();
}

void GlobalController::onError(const QString& error)
{
	m_cout << QDateTime::currentDateTime().toString(Common::dateTimeFormat) << "\n" << error << "\n\n";
	m_cout.flush();
}
