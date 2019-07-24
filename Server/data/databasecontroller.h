#pragma once

#include "common/message.h"
#include "common/person.h"
#include "common/commands.h"

class QWebSocket;

namespace Controllers
{
namespace Data
{

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	DatabaseController();
	~DatabaseController();

public slots:
	void processClientQuery(const QString& query, QWebSocket* socket);

signals:
	void responseReady(const QString& response, QWebSocket* socket);
	void sendCommand(const QString& command, const Common::PersonIdType& userId) const;
	void error(const QString& error) const;
	void saveClientId(const Common::PersonIdType& id, QWebSocket* socket) const;

private:
	QString processRegistrationQuery(const QJsonObject& command, QWebSocket* socket) const;
	QString processLogInRequestQuery(const QJsonObject& command, QWebSocket* socket) const;
	QString processGetLastMessagesQuery(const QJsonObject& command) const;
	QString processSendMessagesQuery(const QJsonObject& command) const;
	QString processGetMessagesQuery(const QJsonObject& command) const;
	QString processFindFriendQuery(const QJsonObject& command) const;

	std::vector<Common::Message> getMessages(const Common::GetMessagesRequest& request) const;
	std::vector<Common::Message> insertMessages(const Common::SendMessagesRequest& request) const;
	
	std::optional<Common::Person> getPerson(Common::PersonIdType id) const;
	std::optional<Common::Person> getPerson(const QString& login, const QString& password) const;
	std::optional<Common::Person> insertPerson(const Common::RegistrationRequest& request) const;

	std::vector<std::pair<Common::Person, Common::Message>> getLastMessages(const Common::GetLastMessagesRequest& request) const;

	std::vector<std::pair<Common::Person, std::optional<Common::Message>>>
		getPossiblePersons(const Common::FindFriendRequest& request) const;

	Common::PersonIdType getLastInsertedPersonId() const;
	Common::MessageIdType getLastInsertedMessageId() const;

	std::optional<QString> initQueries();
	bool createEmptyDatabase();

private:
	QSqlDatabase m_database;
	const QString m_databaseName;

	mutable QSqlQuery m_getMessagesQuery;
	mutable QSqlQuery m_insertMessageQuery;
	mutable QSqlQuery m_getMessageIdQuery;

	mutable QSqlQuery m_selectPersonQuery;
	mutable QSqlQuery m_insertPersonQuery;

	mutable QSqlQuery m_insertAuthInfoQuery;
	mutable QSqlQuery m_checkIfLoginExistsQuery;
	mutable QSqlQuery m_selectPersonFromAuthInfoQuery;

	mutable QSqlQuery m_getLastMessagesQuery;
	mutable QSqlQuery m_findFriendsQuery;

	mutable QSqlQuery m_getLastInsertedIdQuery;
};

}
}