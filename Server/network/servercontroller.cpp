#include "servercontroller.h"

using namespace Controllers::Network;

ServerController::ServerController(quint16 port, QObject *parent)
	: QObject(parent)
	, m_port(port)
{
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

	VERIFY(connect(newSocket, &QWebSocket::disconnected, this, &ServerController::onClientDisconnected));
	VERIFY(connect(newSocket, &QWebSocket::textMessageReceived, this, &ServerController::onMessageReceived));
	VERIFY(connect(newSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onClientError(QAbstractSocket::SocketError))));

	m_clients << newSocket;
}

void ServerController::onThreadStarted()
{
	m_server = std::make_unique<QWebSocketServer>("SunChat Server", QWebSocketServer::NonSecureMode, this);
	
	ASSERT(m_server->listen(QHostAddress::Any, m_port));
	log(QString("SunChat Server is listening on port %1").arg(m_port));

	VERIFY(connect(m_server.get(), &QWebSocketServer::newConnection, this, &ServerController::onNewConnection));
}

void ServerController::onResponseReady(const QString& response, QWebSocket* socket)
{
	socket->sendTextMessage(response);
}

void ServerController::onMessageReceived(const QString& message)
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	ASSERT(client);
	log(client, message);
	
	emit processClientQuery(message, client);
}

void ServerController::onClientError(QAbstractSocket::SocketError)
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	ASSERT(client);

	log(client, QString("error: %1").arg(client->errorString()));
}

void ServerController::onClientDisconnected()
{
	QWebSocket *client = qobject_cast<QWebSocket*>(sender());
	ASSERT(client);
	log(client, "disconnected");

	m_clients.removeAll(client);
	client->deleteLater();
}

void ServerController::log(QWebSocket* client, const QString& text) const
{
	emit error(QString("%1\n[\n%2\n]\n\n").arg(peerInfo(client), text));
}

void ServerController::log(const QString& text) const
{
	emit error(text);
}

QString ServerController::peerInfo(QWebSocket* peer)
{
	return QString("[%1 | %2:%3]").arg(peer->peerName()).arg(peer->peerAddress().toString()).arg(peer->localPort());
}
