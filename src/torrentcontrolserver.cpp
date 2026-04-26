#include "torrentcontrolserver.h"

#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QTcpServer>
#include <QtConcurrent/QtConcurrent>

#include <QEventLoop>
#include <QUrlQuery>

#include <Kanoop/commonexception.h>
#include <Kanoop/torrent/torrentsearcher.h>
#include <Kanoop/torrent/torrentsearchresult.h>

#include "buildinfo.h"
#include "kanooptorrentdaemon.h"
#include "settings.h"

TorrentControlServer* TorrentControlServer::instance()
{
    static TorrentControlServer* _instance = nullptr;
    static QMutex _startLock;

    _startLock.lock();
    if(_instance == nullptr) {
        _instance = new TorrentControlServer;
    }
    _startLock.unlock();
    return _instance;
}

TorrentControlServer::TorrentControlServer() :
    AbstractThreadClass("control-server")
{
}

void TorrentControlServer::threadStarted()
{
    Settings* settings = Settings::instance();
    QString bindAddress = settings->controlBindAddress();
    int listenPort = settings->controlListenPort();

    try
    {
        _httpServer = new QHttpServer;

        _httpServer->route(
                "/admin/health", QHttpServerRequest::Method::Get,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleHealth(request);
            });
        });

        _httpServer->route(
                "/admin/version", QHttpServerRequest::Method::Get,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleVersion(request);
            });
        });

        _httpServer->route(
                "/admin/settings", QHttpServerRequest::Method::Get,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleSettingsGet(request);
            });
        });

        _httpServer->route(
                "/admin/settings", QHttpServerRequest::Method::Put,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleSettingsPut(request);
            });
        });

        _httpServer->route(
                "/torrents/search", QHttpServerRequest::Method::Get,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleSearch(request);
            });
        });

        _httpServer->route(
                "/torrents", QHttpServerRequest::Method::Post,
                [this](const QHttpServerRequest& request) {
            return QtConcurrent::run([this, request]() {
                return this->handleAddTorrent(request);
            });
        });

        _tcpServer = new QTcpServer;
        if(!_tcpServer->listen(QHostAddress(bindAddress), listenPort) || !_httpServer->bind(_tcpServer)) {
            throw CommonException(QString("Control server failed to bind %1:%2").arg(bindAddress).arg(listenPort));
        }

        logText(LVL_INFO, QString("Control server listening on %1:%2").arg(bindAddress).arg(listenPort));
    }
    catch(const CommonException& e)
    {
        logText(LVL_ERROR, e.message());
        finishAndStop(false, e.message());
    }
}

void TorrentControlServer::threadFinished()
{
    delete _httpServer;
    _httpServer = nullptr;
    _tcpServer = nullptr;  // owned by _httpServer's bind, deleted with it

    logText(LVL_INFO, "Control server stopped");
}

QHttpServerResponse TorrentControlServer::handleHealth(const QHttpServerRequest& request)
{
    Q_UNUSED(request)

    QDateTime startedAt;
    qint64 uptimeSeconds = 0;

    QMetaObject::invokeMethod(KanoopTorrentDaemon::instance(), "startedAt",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QDateTime, startedAt));
    QMetaObject::invokeMethod(KanoopTorrentDaemon::instance(), "uptimeSeconds",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(qint64, uptimeSeconds));

    QJsonObject body;
    body["status"] = "ok";
    body["started_at"] = startedAt.toString(Qt::ISODate);
    body["uptime_seconds"] = static_cast<qint64>(uptimeSeconds);

    return QHttpServerResponse(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QHttpServerResponse TorrentControlServer::handleVersion(const QHttpServerRequest& request)
{
    Q_UNUSED(request)

    QJsonObject body;
    body["version"] = BUILD_VERSION_STR;
    body["git_sha"] = BUILD_GIT_SHA_STR;
    body["build_timestamp"] = BUILD_TIMESTAMP_STR;
    body["qt_version"] = qVersion();

    return QHttpServerResponse(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QHttpServerResponse TorrentControlServer::handleSettingsGet(const QHttpServerRequest& request)
{
    Q_UNUSED(request)

    Settings* settings = Settings::instance();
    QJsonObject body;
    body["download_dir"] = settings->downloadDirectory();
    body["resume_dir"] = settings->resumeDataDirectory();
    body["listen_port"] = settings->listenPort();
    body["control_bind_address"] = settings->controlBindAddress();
    body["control_listen_port"] = settings->controlListenPort();

    return QHttpServerResponse(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QHttpServerResponse TorrentControlServer::handleSettingsPut(const QHttpServerRequest& request)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject err;
        err["error"] = QString("Invalid JSON: %1").arg(parseError.errorString());
        return QHttpServerResponse(QJsonDocument(err).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject in = doc.object();
    Settings* settings = Settings::instance();

    QJsonArray applied;
    QJsonArray requiresRestart;

    if(in.contains("download_dir")) {
        settings->setDownloadDirectory(in.value("download_dir").toString());
        applied.append("download_dir");
        requiresRestart.append("download_dir");
    }
    if(in.contains("resume_dir")) {
        settings->setResumeDataDirectory(in.value("resume_dir").toString());
        applied.append("resume_dir");
        requiresRestart.append("resume_dir");
    }
    if(in.contains("listen_port")) {
        settings->setListenPort(in.value("listen_port").toInt());
        applied.append("listen_port");
        requiresRestart.append("listen_port");
    }
    if(in.contains("control_bind_address")) {
        settings->setControlBindAddress(in.value("control_bind_address").toString());
        applied.append("control_bind_address");
        requiresRestart.append("control_bind_address");
    }
    if(in.contains("control_listen_port")) {
        settings->setControlListenPort(in.value("control_listen_port").toInt());
        applied.append("control_listen_port");
        requiresRestart.append("control_listen_port");
    }
    settings->sync();

    QJsonObject body;
    body["applied"] = applied;
    body["requires_restart"] = requiresRestart;
    body["errors"] = QJsonArray();

    return QHttpServerResponse(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QHttpServerResponse TorrentControlServer::handleSearch(const QHttpServerRequest& request)
{
    QString query = QUrlQuery(request.url().query()).queryItemValue("q");
    if(query.isEmpty()) {
        QJsonObject err;
        err["error"] = "missing q parameter";
        return QHttpServerResponse(QJsonDocument(err).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    TorrentSearcher searcher;
    QJsonArray resultsArray;
    QString errorMessage;
    bool succeeded = false;
    QEventLoop loop;

    QObject::connect(&searcher, &TorrentSearcher::searchComplete, &searcher,
                     [&](const QList<TorrentSearchResult>& results) {
        succeeded = true;
        for(const TorrentSearchResult& r : results) {
            QJsonObject row;
            row["name"] = r.name();
            row["info_hash"] = r.infoHash();
            row["size"] = r.size();
            row["size_human"] = TorrentSearchResult::formatSize(r.size());
            row["seeders"] = r.seeders();
            row["leechers"] = r.leechers();
            row["added_date"] = r.addedDate().toString(Qt::ISODate);
            row["category"] = r.category();
            row["uploader_name"] = r.uploaderName();
            row["magnet"] = r.toMagnetLink().toUri();
            resultsArray.append(row);
        }
        loop.quit();
    });
    QObject::connect(&searcher, &TorrentSearcher::searchFailed, &searcher,
                     [&](const QString& msg) {
        errorMessage = msg;
        loop.quit();
    });

    searcher.search(query);
    loop.exec();

    if(!succeeded) {
        QJsonObject err;
        err["error"] = errorMessage;
        return QHttpServerResponse(QJsonDocument(err).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadGateway);
    }

    QJsonObject body;
    body["query"] = query;
    body["results"] = resultsArray;
    return QHttpServerResponse(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QHttpServerResponse TorrentControlServer::handleAddTorrent(const QHttpServerRequest& request)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject err;
        err["error"] = QString("Invalid JSON: %1").arg(parseError.errorString());
        return QHttpServerResponse(QJsonDocument(err).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    QString magnet = doc.object().value("magnet").toString();
    if(magnet.isEmpty()) {
        QJsonObject err;
        err["error"] = "missing magnet";
        return QHttpServerResponse(QJsonDocument(err).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject result;
    QMetaObject::invokeMethod(KanoopTorrentDaemon::instance(), "addMagnet",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QJsonObject, result),
                              Q_ARG(QString, magnet));

    if(result.contains("error")) {
        return QHttpServerResponse(QJsonDocument(result).toJson(QJsonDocument::Compact),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    return QHttpServerResponse(QJsonDocument(result).toJson(QJsonDocument::Compact));
}
