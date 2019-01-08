#pragma once

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QString>

namespace Database
{

class DatabaseController
{
public:
	DatabaseController();
	~DatabaseController();

private:
	bool createEmptyDatabase();

private:
	QSqlDatabase m_database;
	const QString m_databaseName;
	QString m_lastError;
};

}
