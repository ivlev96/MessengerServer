#include "databasecontroller.h"

#include <QTextStream>
#include <QFile>
#include <QJsonDocument>

using namespace Data;
using namespace Common;

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
			QTextStream(stdout) << m_lastError;
			return;
		}
	}
	assert(m_database.open());

	if (!initQueries())
	{
		return;
	}

#ifdef _DEBUG
	if (needToCreateDb)
	{
		if (!debugFillEmptyDatabase())
		{
			return;
		}
	}

	QJsonArray all = getMessages(1, 2, 3, 2);
	QJsonDocument doc(all);

	QFile debugMessages1("debugMessages1.json");
	assert(debugMessages1.open(QFile::WriteOnly));

	debugMessages1.write(doc.toJson());
	debugMessages1.close();
#endif
}

DatabaseController::~DatabaseController()
{
	m_database.close();
}

QString DatabaseController::lastError() const
{
	return m_lastError;
}

QJsonArray DatabaseController::getMessages(int id1, int id2, std::size_t count, std::size_t from) const
{
	std::map<int, Person> person;

	person[id1] = getPersonRecord(id1);
	person[id2] = getPersonRecord(id2);

	m_selectQuery.bindValue(":id1", id1);
	m_selectQuery.bindValue(":id2", id2);
	m_selectQuery.bindValue(":count", count);
	m_selectQuery.bindValue(":from", from);

	if (!m_selectQuery.exec())
	{
		QString s = m_selectQuery.lastError().text();
		return {};
	}

	QJsonArray json;

	while (m_selectQuery.next())
	{
		const int idFrom = m_selectQuery.value(1).toInt();
		const int idTo = m_selectQuery.value(2).toInt();

		const QDateTime dateTime = 
			QDateTime::fromString(m_selectQuery.value(0).toString(), "dd.MM.yyyy hh:mm:ss");

		const QString message = m_selectQuery.value(3).toString();

		json.push_back(
			Message(
				person[idFrom],
				person[idTo],
				dateTime,
				message
			).toJson());
	}

	return json;
}

bool DatabaseController::insertMessages(const QJsonArray& messages)
{
	for (const auto& message : messages)
	{
		Person from(message["from"].toObject());
		Person to(message["to"].toObject());
		m_insertQuery.bindValue(":idFrom", from.id);
		m_insertQuery.bindValue(":idTo", to.id);
		m_insertQuery.bindValue(":messageDateTime", message["dateTime"]);
		m_insertQuery.bindValue(":messageText", message["text"]);

		if (!m_insertQuery.exec())
		{
			m_lastError = m_insertQuery.lastError().text();
			return false;
		}
	}

	return true;
}

bool DatabaseController::insertPerson(const QJsonObject& person)
{
	m_insertPersonQuery.bindValue(":firstName", person["firstName"]);
	m_insertPersonQuery.bindValue(":lastName", person["lastName"]);
	m_insertPersonQuery.bindValue(":avatarUrl", person["avatarUrl"]);

	if (!m_insertPersonQuery.exec())
	{
		m_lastError = m_insertPersonQuery.lastError().text();
		return false;
	}

	return true;
}

bool DatabaseController::initQueries()
{
	assert(m_database.isOpen());

	m_insertQuery = QSqlQuery(m_database);
	m_selectQuery = QSqlQuery(m_database);
	m_insertPersonQuery = QSqlQuery(m_database);
	m_selectPersonQuery = QSqlQuery(m_database);

	if (!m_insertQuery.prepare(
		"INSERT INTO Messages "
		"	(idFrom, idTo, messageDateTime, messageText) "
		"VALUES "
		"	(:idFrom, :idTo, :messageDateTime, :messageText)"))
	{
		m_lastError = m_insertQuery.lastError().text();
		return false;
	}

	if (!m_selectQuery.prepare(
		"SELECT * "
		"	FROM (SELECT messageDateTime, idFrom, idTo, messageText "
		"		FROM Messages "
		"		WHERE idFrom == :id1 AND idTo == :id2 "
		"			OR idFrom == :id2 AND idTo == :id1 "
		"		ORDER BY messageDateTime DESC "
		"	LIMIT :from, :count) as result "
		"ORDER BY result.messageDateTime ASC"))
	{
		m_lastError = m_insertQuery.lastError().text();
		return false;
	}

	if (!m_selectPersonQuery.prepare(
		"SELECT firstName, lastName, avatarUrl "
		"	FROM Person "
		"	WHERE id == :id"))
	{
		m_lastError = m_insertQuery.lastError().text();
		return false;
	}

	if (!m_insertPersonQuery.prepare(
		"INSERT INTO Person "
		"	(firstName, lastName, avatarUrl) "
		"VALUES "
		"	(:firstName, :lastName, :avatarUrl)"))
	{
		m_lastError = m_insertQuery.lastError().text();
		return false;
	}

	return true;
}

bool DatabaseController::createEmptyDatabase()
{
	QTextStream cout(stdout);

	m_database.setDatabaseName(m_databaseName);

	if (!m_database.open())
	{
		m_lastError = m_database.lastError().text();
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
		m_lastError = query.lastError().text();
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
		m_lastError = query.lastError().text();
		m_database.close();
		return false;
	}

	m_database.close();
	return true;
}

bool DatabaseController::debugFillEmptyDatabase()
{
	QFile debugMessages("debugMessages.json");
	assert(debugMessages.open(QFile::ReadOnly | QFile::Text));

	QJsonDocument doc = QJsonDocument::fromJson(debugMessages.readAll());
	debugMessages.close();

	assert(doc.isArray());
	if (!insertMessages(doc.array()))
	{
		return false;
	}

	if (!insertPerson(Person(1, "Ivan", "Ivlev", QUrl::fromLocalFile("Vanya.jpg").toString()).toJson()))
	{
		return false;
	}

	if (!insertPerson(Person(2, "Pavel", "Zharov", QUrl::fromLocalFile("Pasha.jpg").toString()).toJson()))
	{
		return false;
	}

	return true;
}

Person DatabaseController::getPersonRecord(int id) const
{
	m_selectPersonQuery.bindValue(":id", id);

	if (!m_selectPersonQuery.exec())
	{
		m_lastError = m_selectPersonQuery.lastError().text();
		return {};
	}

	if (!m_selectPersonQuery.next())
	{
		m_lastError = m_selectPersonQuery.lastError().text();
		return {};
	}

	return 
	{
		id,
		m_selectPersonQuery.value(0).toString(),
		m_selectPersonQuery.value(1).toString(),
		m_selectPersonQuery.value(2).toString()
	};
}
