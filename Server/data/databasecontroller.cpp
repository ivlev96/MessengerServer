#include "databasecontroller.h"
#include "common/commands.h"
#include "common/common.h"

using namespace Controllers::Data;
DatabaseController::DatabaseController()
	: m_databaseName("db.sqlite")
{
	m_database = QSqlDatabase::addDatabase("QSQLITE");
	m_database.setDatabaseName(m_databaseName);

	const bool needToCreateDb = !QFile(m_databaseName).exists();
	if (needToCreateDb)
	{
		if (!createEmptyDatabase())
		{
			return;
		}
	}
	VERIFY(m_database.open());

	const auto error = initQueries();
	if (error.has_value())
	{
		emit this->error(*error);
	}
}

DatabaseController::~DatabaseController()
{
	m_database.close();
}

void DatabaseController::processClientQuery(const QString& query, QWebSocket* socket)
{
	QJsonParseError error;
	const QJsonDocument doc = QJsonDocument::fromJson(query.toUtf8(), &error);
	ASSERT(error.error == QJsonParseError::NoError);
	ASSERT(doc.isObject());
	
	QJsonObject json = doc.object();

	ASSERT(json[Common::typeField].isString());
	const QString type = json[Common::typeField].toString();

	if (type == Common::registrationRequest)
	{
		emit responseReady(processRegistrationQuery(json, socket), socket);
	}
	else if (type == Common::logInRequest)
	{
		emit responseReady(processLogInRequestQuery(json, socket), socket);
	}
	else if (type == Common::getLastMessagesRequest)
	{
		emit responseReady(processGetLastMessagesQuery(json), socket);
	}
	else if (type == Common::sendMessagesRequest)
	{
		emit responseReady(processSendMessagesQuery(json), socket);
	}
	else if (type == Common::getMessagesRequest)
	{
		emit responseReady(processGetMessagesQuery(json), socket);
	}
	else
	{
		ASSERT(!"Not implemented");
	}
}

QString DatabaseController::processRegistrationQuery(const QJsonObject& command, QWebSocket* socket) const
{
	const Common::RegistrationRequest request(command);
	const Common::RegistrationResponse response(insertPerson(request));

	if (response.person)
	{
		emit saveClientId(response.person->id, socket);
	}

	return Common::toString(response);
}

QString DatabaseController::processLogInRequestQuery(const QJsonObject& command, QWebSocket* socket) const
{
	const Common::LogInRequest request(command);
	const Common::LogInResponse response(getPerson(request.login, request.password));

	if (response.person)
	{
		emit saveClientId(response.person->id, socket);
	}

	return Common::toString(response);
}

QString DatabaseController::processGetLastMessagesQuery(const QJsonObject& command) const
{
	const Common::GetLastMessagesRequest request(command);
	const Common::GetLastMessagesResponse response(request.id, getLastMessages(request), request.before);

	return Common::toString(response);
}

QString DatabaseController::processSendMessagesQuery(const QJsonObject& command) const
{
	const Common::SendMessagesRequest request(command);
	const Common::SendMessagesResponse response(insertMessages(request));

	for (auto& message : response.messages)
	{
		const auto from = getPerson(message.from);
		ASSERT(from);
		const Common::NewMessageCommand newMessage(*from, message);
		emit sendCommand(Common::toString(newMessage), message.to);
	}

	return Common::toString(response);
}

QString DatabaseController::processGetMessagesQuery(const QJsonObject& command) const
{
	const Common::GetMessagesRequest request(command);
	const Common::GetMessagesResponse response(request.id1, request.id2, getMessages(request), request.before);

	return Common::toString(response);
}

std::vector<Common::Message> DatabaseController::getMessages(const Common::GetMessagesRequest& request) const
{
	std::map<Common::PersonIdType, Common::Person> person;

	person[request.id1] = *getPerson(request.id1);
	person[request.id2] = *getPerson(request.id2);

	m_getMessagesQuery.bindValue(":id1", request.id1);
	m_getMessagesQuery.bindValue(":id2", request.id2);
	m_getMessagesQuery.bindValue(":count", request.count);
	m_getMessagesQuery.bindValue(":before", request.before.has_value() ? *request.before : 0);
	m_getMessagesQuery.bindValue(":isNew", !request.before.has_value());

	if (!m_getMessagesQuery.exec())
	{
		emit error(m_getMessagesQuery.lastError().text());
		return {};
	}

	std::vector<Common::Message> messages;

	while (m_getMessagesQuery.next())
	{
		const Common::MessageIdType id = m_getMessagesQuery.value(0).toInt();
		const QDateTime dateTime = m_getMessagesQuery.value(1).toDateTime();
		ASSERT(dateTime.isValid());

		const Common::PersonIdType idFrom = m_getMessagesQuery.value(2).toInt();
		const Common::PersonIdType idTo = m_getMessagesQuery.value(3).toInt();

		const QString text = m_getMessagesQuery.value(4).toString();

		messages.emplace_back(
				idFrom,
				idTo,
				dateTime,
				text,
				id,
				Common::Message::State::Sent);
	}

	return messages;
}

std::vector<Common::Message> DatabaseController::insertMessages(const Common::SendMessagesRequest& request) const
{
	std::vector<Common::Message> messages;
	messages.reserve(request.messages.size());

	for (auto message : request.messages)
	{
		m_insertMessageQuery.bindValue(":idFrom", message.from);
		m_insertMessageQuery.bindValue(":idTo", message.to);
		m_insertMessageQuery.bindValue(":messageDateTime", message.dateTime);
		m_insertMessageQuery.bindValue(":messageText", message.text);

		if (!m_insertMessageQuery.exec())
		{
			emit error(m_insertMessageQuery.lastError().text());
		}
		else
		{
			message.id = getLastInsertedMessageId();
			ASSERT(*message.id >= 0);
			message.state = Common::Message::State::Sent;
		}
		
		messages.push_back(message);
	}

	return messages;
}

std::optional<Common::Person> DatabaseController::getPerson(Common::PersonIdType id) const
{
	m_selectPersonQuery.bindValue(":id", id);

	if (!m_selectPersonQuery.exec())
	{
		emit error(m_selectPersonQuery.lastError().text());
		return {};
	}

	if (m_selectPersonQuery.next())
	{
		return Common::Person(
			id,
			m_selectPersonQuery.value(0).toString(),
			m_selectPersonQuery.value(1).toString(),
			m_selectPersonQuery.value(2).toString()
		);
	}

	return {};
}

std::optional<Common::Person> DatabaseController::getPerson(const QString& login, const QString& password) const
{
	m_selectPersonFromAuthInfoQuery.bindValue(":login", login);
	m_selectPersonFromAuthInfoQuery.bindValue(":password", password);

	if (!m_selectPersonFromAuthInfoQuery.exec())
	{
		emit error(m_selectPersonFromAuthInfoQuery.lastError().text());
		return {};
	}

	if (m_selectPersonFromAuthInfoQuery.next())
	{
		return Common::Person(
			m_selectPersonFromAuthInfoQuery.value(0).toInt(),
			m_selectPersonFromAuthInfoQuery.value(1).toString(),
			m_selectPersonFromAuthInfoQuery.value(2).toString(),
			m_selectPersonFromAuthInfoQuery.value(3).toString()
		);
	}

	return {};
}

std::optional<Common::Person> DatabaseController::insertPerson(const Common::RegistrationRequest& request) const
{
	m_checkIfLoginExistsQuery.bindValue(":login", request.login);

	if (!m_checkIfLoginExistsQuery.exec())
	{
		emit error(m_checkIfLoginExistsQuery.lastError().text());
		return {};
	}

	if (!m_checkIfLoginExistsQuery.next())
	{
		return {};
	}

	const bool exists = m_checkIfLoginExistsQuery.value(0).toBool();
	if (exists)
	{
		return {};
	}

	m_insertPersonQuery.bindValue(":firstName", request.firstName);
	m_insertPersonQuery.bindValue(":lastName", request.lastName);
	m_insertPersonQuery.bindValue(":avatarUrl", request.avatarUrl);

	if (!m_insertPersonQuery.exec())
	{
		emit error(m_insertPersonQuery.lastError().text());
		return {};
	}

	const Common::PersonIdType id = getLastInsertedPersonId();

	m_insertAuthInfoQuery.bindValue(":login", request.login);
	m_insertAuthInfoQuery.bindValue(":password", request.password);
	m_insertAuthInfoQuery.bindValue(":personId", id);

	if (!m_insertAuthInfoQuery.exec())
	{
		emit error(m_insertAuthInfoQuery.lastError().text());
		return {};
	}

	return Common::Person(
		id,
		request.firstName,
		request.lastName,
		request.avatarUrl
	);
}

std::vector<std::pair<Common::Person, Common::Message>> DatabaseController::getLastMessages(const Common::GetLastMessagesRequest& request) const
{
	m_getLastMessagesQuery.bindValue(":id", request.id);
	m_getLastMessagesQuery.bindValue(":count", request.count);
	m_getLastMessagesQuery.bindValue(":before", request.before.has_value() ? *request.before : 0);
	m_getLastMessagesQuery.bindValue(":isNew", !request.before.has_value());

	if (!m_getLastMessagesQuery.exec())
	{
		emit error(m_getLastMessagesQuery.lastError().text());
		return {};
	}

	std::vector<std::pair<Common::Person, Common::Message>> messages;

	while (m_getLastMessagesQuery.next())
	{
		const Common::MessageIdType messageId = m_getLastMessagesQuery.value(0).toInt();
		const int idFrom = m_getLastMessagesQuery.value(1).toInt();
		const int idTo = m_getLastMessagesQuery.value(2).toInt();

		const QDateTime dateTime =
			QDateTime::fromString(m_getLastMessagesQuery.value(3).toString(), Common::dateTimeFormat);

		const QString text = m_getLastMessagesQuery.value(4).toString();

		const Common::Message message(idFrom, idTo, dateTime, text, messageId, Common::Message::State::Sent);

		const Common::PersonIdType otherId = m_getLastMessagesQuery.value(5).toInt();
		const QString firstName = m_getLastMessagesQuery.value(6).toString();
		const QString lastName = m_getLastMessagesQuery.value(7).toString();
		const QString avatarUrl = m_getLastMessagesQuery.value(8).toString();

		const Common::Person other(otherId, firstName, lastName, avatarUrl);

		messages.emplace_back(other, message);
	}

	return messages;
}

Common::PersonIdType Controllers::Data::DatabaseController::getLastInsertedPersonId() const
{
	if (!m_getLastInsertedIdQuery.exec())
	{
		emit error(m_getLastInsertedIdQuery.lastError().text());
	}

	if (m_getLastInsertedIdQuery.next())
	{
		return m_getLastInsertedIdQuery.value(0).toInt();
	}

	return -1;
}

Common::MessageIdType Controllers::Data::DatabaseController::getLastInsertedMessageId() const
{
	if (!m_getLastInsertedIdQuery.exec())
	{
		emit error(m_getLastInsertedIdQuery.lastError().text());
	}

	if (m_getLastInsertedIdQuery.next())
	{
		return m_getLastInsertedIdQuery.value(0).toInt();
	}

	return -1;
}

std::optional<QString> DatabaseController::initQueries()
{
	ASSERT(m_database.isOpen());

	m_getMessagesQuery = QSqlQuery(m_database);
	m_insertMessageQuery = QSqlQuery(m_database);
	m_getMessageIdQuery = QSqlQuery(m_database);

	m_selectPersonQuery = QSqlQuery(m_database);
	m_insertPersonQuery = QSqlQuery(m_database);

	m_insertAuthInfoQuery = QSqlQuery(m_database);
	m_checkIfLoginExistsQuery = QSqlQuery(m_database);
	m_selectPersonFromAuthInfoQuery = QSqlQuery(m_database);

	m_getLastMessagesQuery = QSqlQuery(m_database);

	m_getLastInsertedIdQuery = QSqlQuery(m_database);

	if (!m_insertMessageQuery.prepare(
		"INSERT INTO Messages "
		"	(idFrom, idTo, messageDateTime, messageText) "
		"VALUES "
		"	(:idFrom, :idTo, :messageDateTime, :messageText)"))
	{
		return m_insertMessageQuery.lastError().text();
	}

	if (!m_getMessagesQuery.prepare(
		"SELECT * "
		"	FROM (SELECT id, messageDateTime, idFrom, idTo, messageText "
		"		FROM Messages "
		"		WHERE (idFrom = :id1 AND idTo = :id2 "
		"			OR idFrom = :id2 AND idTo = :id1) "
		"			AND (:isNew OR id < :before) "			
		"	ORDER BY id DESC "
		"	LIMIT :count) as result "
		"ORDER BY id ASC"))
	{
		return m_getMessagesQuery.lastError().text();
	}

	if (!m_getMessageIdQuery.prepare(
		"SELECT id "
		"	FROM Messages "
		"	WHERE idFrom = :idFrom "
		"		AND idTo = :idTo "
		"		AND messageDateTime = :messageDateTime "
		"		AND messageText = :messageText "
		"ORDER BY id DESC "
		"LIMIT 1"))
	{
		return m_getMessageIdQuery.lastError().text();
	}

	if (!m_selectPersonQuery.prepare(
		"SELECT firstName, lastName, avatarUrl "
		"	FROM Person "
		"	WHERE id = :id"))
	{
		return m_selectPersonQuery.lastError().text();
	}

	if (!m_insertPersonQuery.prepare(
		"INSERT INTO Person "
		"	(firstName, lastName, avatarUrl) "
		"VALUES "
		"	(:firstName, :lastName, :avatarUrl)"))
	{
		return m_insertPersonQuery.lastError().text();
	}

	if (!m_insertAuthInfoQuery.prepare(
		"INSERT INTO Auth "
		"	(login, password, personId) "
		"VALUES "
		"	(:login, :password, :personId)"))
	{
		return m_insertAuthInfoQuery.lastError().text();
	}

	if (!m_checkIfLoginExistsQuery.prepare(
		"SELECT EXISTS (SELECT * "
		"	FROM Auth "
		"	WHERE login = :login)"))
	{
		return m_checkIfLoginExistsQuery.lastError().text();
	}

	if (!m_selectPersonFromAuthInfoQuery.prepare(
		"SELECT id, firstName, lastName, avatarUrl "
		"	FROM Person "
		"	WHERE id = (SELECT personId "
		"		FROM Auth "
		"		WHERE login = :login "
		"			AND password = :password)"))
	{
		return m_selectPersonFromAuthInfoQuery.lastError().text();
	}

	if (!m_getLastMessagesQuery.prepare(
		"SELECT m.id, m.idFrom, m.idTo, m.messageDateTime, m.messageText, p.id, p.firstName, p.lastName, p.avatarUrl "
		"    FROM "
		"        (SELECT otherId, max(id) as id "
		"            FROM "
		"                (SELECT idTo as otherId, * "
		"                    FROM Messages "
		"                WHERE idFrom = :id	 "
		"                UNION "
		"                SELECT idFrom as otherId, * "
		"                    FROM Messages "
		"                WHERE idTo = :id) "
		"        GROUP BY otherId) filtered "
		"    JOIN Messages m "
		"    ON m.id = filtered.id "
		"        AND (m.id < :before OR :isNew) "
		"    LEFT JOIN Person p "
		"    ON filtered.otherId = p.id "
		"ORDER BY m.id DESC "
		"LIMIT :count "))
	{
		return m_getLastMessagesQuery.lastError().text();
	}

	if (!m_getLastInsertedIdQuery.prepare(
		"SELECT last_insert_rowid()"))
	{
		return m_getLastInsertedIdQuery.lastError().text();
	}

	return {};
}

bool DatabaseController::createEmptyDatabase()
{
	QTextStream cout(stdout);

	m_database.setDatabaseName(m_databaseName);

	if (!m_database.open())
	{
		emit error(m_database.lastError().text());
		return false;
	}

	QSqlQuery query;

	if (!query.exec(
		"create table Messages "
		"(id integer primary key, "
		"idFrom integer, "
		"idTo integer, "
		"messageDateTime datetime, "
		"messageText varchar(10))"))
	{
		emit error(query.lastError().text());
		m_database.close();
		return false;
	}

	if (!query.exec(
		"create table Person "
		"(id integer primary key, "
		"firstName varchar(10), "
		"lastName varchar(10), "
		"avatarUrl varchar(10))"))
	{
		emit error(query.lastError().text());
		m_database.close();
		return false;
	}

	if (!query.exec(
		"create table Auth "
		"(id integer primary key, "
		"login varchar(10), "
		"password varchar(10), "
		"personId integer)"))
	{
		emit error(query.lastError().text());
		m_database.close();
		return false;
	}

	m_database.close();
	return true;
}

