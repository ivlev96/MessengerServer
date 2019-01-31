#pragma once

#include "common/common.h"
#include "common/commands.h"
#include "common/person.h"

#include <QObject>
#include <QtWebSockets>
#include <QList>
#include <QString>
#include <QTextStream>

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
	mutable QTextStream m_log;
	QWebSocketServer* m_server;
	QList<QWebSocket*> m_clients;
};

}
}