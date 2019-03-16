#pragma once

#include "common/common.h"
#include "common/commands.h"
#include "common/person.h"

namespace Controllers
{
namespace Network
{

class ServerController : public QObject
{
	Q_OBJECT

public:
	ServerController(quint16 port, QObject *parent = nullptr);
	~ServerController();

signals:
	void error(const QString& error) const;
	void processClientQuery(const QString& query, QWebSocket* socket);

public slots:
	void onThreadStarted();
	void onSaveClientId(const Common::PersonIdType& id, QWebSocket* socket);
	void onResponseReady(const QString& response, QWebSocket* socket);
	void onSendCommand(const QString& command, const Common::PersonIdType& userId);

private slots:
	void onNewConnection();
	void onMessageReceived(const QString& message);
	void onClientError(QAbstractSocket::SocketError error);
	void onClientDisconnected();

private:
	void log(QWebSocket* client, const QString& text) const;
	void log(const QString& text) const;
	static QString peerInfo(QWebSocket* peer);

private:
	quint16 m_port;
	std::unique_ptr<QWebSocketServer> m_server;
	QList<QWebSocket*> m_clients;
	QMap<Common::PersonIdType, QWebSocket*> m_socketById;
};

}
}