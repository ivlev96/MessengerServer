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
	qRegisterMetaType<Common::PersonIdType>("Common::PersonIdType");
	m_server->moveToThread(m_serverThread);
	VERIFY(connect(m_serverThread, &QThread::finished, m_server, &QObject::deleteLater));
	VERIFY(connect(m_serverThread, &QThread::started, m_server, &ServerController::onThreadStarted));

	VERIFY(connect(m_server, &ServerController::processClientQuery, m_db.get(), &DatabaseController::processClientQuery));
	VERIFY(connect(m_db.get(), &DatabaseController::saveClientId, m_server, &ServerController::onSaveClientId));
	VERIFY(connect(m_db.get(), &DatabaseController::responseReady, m_server, &ServerController::onResponseReady));
	VERIFY(connect(m_db.get(), &DatabaseController::sendCommand, m_server, &ServerController::onSendCommand));

	//errors
	VERIFY(connect(m_server, &ServerController::error, this, &GlobalController::onError));
	VERIFY(connect(m_db.get(), &DatabaseController::error, this, &GlobalController::onError));

	m_serverThread->start();
	ASSERT(m_serverThread->isRunning());
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
