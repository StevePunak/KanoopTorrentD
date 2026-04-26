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
#include "json/adminbodies.h"
#include "json/operationresult.h"
#include "json/torrentbodies.h"
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

namespace {

QHttpServerResponse errorResponse(const QString& message,
                                  QHttpServerResponder::StatusCode code = QHttpServerResponder::StatusCode::BadRequest)
{
    return QHttpServerResponse(OperationResult(false, message).serializeToJson(), code);
}

} // namespace

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

    return QHttpServerResponse(HealthResponseBody("ok", startedAt, uptimeSeconds).serializeToJson());
}

QHttpServerResponse TorrentControlServer::handleVersion(const QHttpServerRequest& request)
{
    Q_UNUSED(request)

    return QHttpServerResponse(VersionResponseBody(BUILD_VERSION_STR, BUILD_GIT_SHA_STR,
                                                   BUILD_TIMESTAMP_STR, qVersion()).serializeToJson());
}

QHttpServerResponse TorrentControlServer::handleSettingsGet(const QHttpServerRequest& request)
{
    Q_UNUSED(request)

    Settings* settings = Settings::instance();
    SettingsBody body;
    body.setDownloadDirectory(settings->downloadDirectory());
    body.setResumeDataDirectory(settings->resumeDataDirectory());
    body.setListenPort(settings->listenPort());
    body.setControlBindAddress(settings->controlBindAddress());
    body.setControlListenPort(settings->controlListenPort());
    return QHttpServerResponse(body.serializeToJson());
}

QHttpServerResponse TorrentControlServer::handleSettingsPut(const QHttpServerRequest& request)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return errorResponse(QString("Invalid JSON: %1").arg(parseError.errorString()));
    }

    SettingsBody updates;
    updates.deserializeFromJsonObject(doc.object());

    Settings* settings = Settings::instance();
    SettingsUpdateResultBody result;

    if(updates.hasDownloadDirectory()) {
        settings->setDownloadDirectory(updates.downloadDirectory());
        result.addApplied("download_dir");
        result.addRequiresRestart("download_dir");
    }
    if(updates.hasResumeDataDirectory()) {
        settings->setResumeDataDirectory(updates.resumeDataDirectory());
        result.addApplied("resume_dir");
        result.addRequiresRestart("resume_dir");
    }
    if(updates.hasListenPort()) {
        settings->setListenPort(updates.listenPort());
        result.addApplied("listen_port");
        result.addRequiresRestart("listen_port");
    }
    if(updates.hasControlBindAddress()) {
        settings->setControlBindAddress(updates.controlBindAddress());
        result.addApplied("control_bind_address");
        result.addRequiresRestart("control_bind_address");
    }
    if(updates.hasControlListenPort()) {
        settings->setControlListenPort(updates.controlListenPort());
        result.addApplied("control_listen_port");
        result.addRequiresRestart("control_listen_port");
    }
    settings->sync();

    return QHttpServerResponse(result.serializeToJson());
}

QHttpServerResponse TorrentControlServer::handleSearch(const QHttpServerRequest& request)
{
    QString query = QUrlQuery(request.url().query()).queryItemValue("q");
    if(query.isEmpty()) {
        return errorResponse("missing q parameter");
    }

    TorrentSearcher searcher;
    SearchResponseBody body;
    body.setQuery(query);
    QString errorMessage;
    bool succeeded = false;
    QEventLoop loop;

    QObject::connect(&searcher, &TorrentSearcher::searchComplete, &searcher,
                     [&](const QList<TorrentSearchResult>& results) {
        succeeded = true;
        for(const TorrentSearchResult& r : results) {
            body.appendResult(SearchResultRow(r));
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
        return errorResponse(errorMessage, QHttpServerResponder::StatusCode::BadGateway);
    }

    return QHttpServerResponse(body.serializeToJson());
}

QHttpServerResponse TorrentControlServer::handleAddTorrent(const QHttpServerRequest& request)
{
    AddTorrentRequestBody requestBody(request.body());
    if(requestBody.magnet().isEmpty()) {
        return errorResponse("missing magnet");
    }

    QJsonObject result;
    QMetaObject::invokeMethod(KanoopTorrentDaemon::instance(), "addMagnet",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QJsonObject, result),
                              Q_ARG(QString, requestBody.magnet()));

    OperationResult opResult;
    opResult.deserializeFromJsonObject(result);
    if(!opResult.success() && result.contains("success")) {
        return QHttpServerResponse(opResult.serializeToJson(),
                                   QHttpServerResponder::StatusCode::BadRequest);
    }

    return QHttpServerResponse(QJsonDocument(result).toJson(QJsonDocument::Compact));
}
