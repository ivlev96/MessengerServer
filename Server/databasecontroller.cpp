#include "databasecontroller.h"
#include <QTextStream>
#include <QFile>

using namespace Database;


DatabaseController::DatabaseController()
	: m_databaseName("db.sqlite")
{
	m_database = QSqlDatabase::addDatabase("QSQLITE");
	m_database.setDatabaseName(m_databaseName);

	if (!QFile(m_databaseName).exists())
	{
		if (createEmptyDatabase())
		{
			QTextStream(stdout) << m_lastError;
		}
	}
}

DatabaseController::~DatabaseController()
{
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
		return false;
	}

	if (!query.exec(
		"create table Person "
		"(id integer primary key, "
		"firstName varchar(10), "
		"secondName varchar(10), "
		"avatarUrl varchar(10))"))
	{
		m_lastError = query.lastError().text();
		return false;
	}

	m_database.close();
	return true;
}
