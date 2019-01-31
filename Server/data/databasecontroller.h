#pragma once

#include "common/message.h"
#include "common/person.h"

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QDir>
#include <QString>

namespace Controllers
{
namespace Data
{

class DatabaseController
{
public:
	DatabaseController();
	~DatabaseController();

	QString lastError() const;

	QJsonArray getMessages(int id1, int id2, std::size_t count, std::size_t from = 0) const;
	bool insertMessages(const QJsonArray& messages);
	bool insertPerson(const QJsonObject& person);

private:
	bool initQueries();
	bool createEmptyDatabase();

#ifdef _DEBUG
	bool debugFillEmptyDatabase();
#endif

	Common::Person getPersonRecord(int id) const;

private:
	QSqlDatabase m_database;
	const QString m_databaseName;
	mutable QString m_lastError;

	mutable QSqlQuery m_selectQuery;
	mutable QSqlQuery m_insertQuery;

	mutable QSqlQuery m_selectPersonQuery;
	mutable QSqlQuery m_insertPersonQuery;
};

}
}