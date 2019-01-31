#include "servercontroller.h"

using namespace Controllers::Network;

ServerController::ServerController(quint16 port, QObject *parent)
	: QObject(parent)
	, m_log(stdout)
	, m_server(new QWebSocketServer("SunChat Server", QWebSocketServer::NonSecureMode, this))
{
	assert(m_server->listen(QHostAddress::Any, port));
	log(QString("SunChat Server is listening on port %1").arg(port));

	assert(connect(m_server, &QWebSocketServer::newConnection, this, &ServerController::onNewConnection));
}

ServerController::~ServerController()
{
	m_server->close();
}

void ServerController::onNewConnection()
{
	QWebSocket* newSocket = m_server->nextPendingConnection();
	log(newSocket, "connected");

	newSocket->setParent(this);

	assert(connect(newSocket, &QWebSocket::disconnected, this, &ServerController::onClientDisconnected));
	assert(connect(newSocket, &QWebSocket::textMessageReceived, this, &ServerController::onMessageReceived));
	assert(connect(newSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onClientError(QAbstractSocket::SocketError))));

	m_clients << newSocket;
}

void ServerController::onMessageReceived(const QString& message)
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	assert(client);
	log(client, message);
}

void ServerController::onClientError(QAbstractSocket::SocketError)
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	assert(client);

	log(client, QString("error: %1").arg(client->errorString()));
}

void ServerController::onClientDisconnected()
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	assert(client);
	log(client, "disconnected");

	m_clients.removeAll(client);
	client->deleteLater();
}

void ServerController::log(QWebSocket* client, const QString& text) const
{
	m_log << peerInfo(client) << "\n[\n" << text << "\n]\n\n";
	m_log.flush();
}

void ServerController::log(const QString& text) const
{
	m_log << text << "\n\n";
	m_log.flush();
}

QString ServerController::peerInfo(QWebSocket* peer)
{
	return QString("[%1 | %2:%3]").arg(peer->peerName()).arg(peer->peerAddress().toString()).arg(peer->localPort());
}
