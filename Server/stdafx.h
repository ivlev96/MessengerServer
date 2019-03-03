#include <vector>
#include <deque>
#include <queue>
#include <optional>

#include <QCoreApplication>
#include <QObject>
#include <QMetaType>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <QUrl>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtQml/QQmlContext>

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

#ifdef _DEBUG

#define ASSERT(X) _ASSERT(X)
#define VERIFY(X) _ASSERT(X)

#else

#define ASSERT(X)
#define VERIFY(X) (X)

#endif