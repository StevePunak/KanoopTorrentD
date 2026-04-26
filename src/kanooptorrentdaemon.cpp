#include "kanooptorrentdaemon.h"

#include <QDir>
#include <QMutex>

#include <Kanoop/torrent/torrentclient.h>

#include "settings.h"
#include "torrentcontrolserver.h"

KanoopTorrentDaemon* KanoopTorrentDaemon::instance()
{
    static KanoopTorrentDaemon* _instance = nullptr;
    static QMutex _startLock;

    _startLock.lock();
    if(_instance == nullptr) {
        _instance = new KanoopTorrentDaemon;
    }
    _startLock.unlock();
    return _instance;
}

KanoopTorrentDaemon::KanoopTorrentDaemon() :
    AbstractThreadClass("torrent-daemon")
{
}

qint64 KanoopTorrentDaemon::uptimeSeconds() const
{
    if(_startedAt.isValid() == false) {
        return 0;
    }
    return _startedAt.secsTo(QDateTime::currentDateTimeUtc());
}

void KanoopTorrentDaemon::threadStarted()
{
    Settings* settings = Settings::instance();
    QString downloadDir = settings->downloadDirectory();
    QString resumeDir = settings->resumeDataDirectory();
    QDir().mkpath(downloadDir);
    QDir().mkpath(resumeDir);

    _client = new TorrentClient();
    _client->setDefaultDownloadDirectory(downloadDir);
    _client->setResumeDataDirectory(resumeDir);
    _client->setListenInterfaces(QString("0.0.0.0:%1,[::]:%1").arg(settings->listenPort()));
    _client->loadResumeData();

    connect(_client, &TorrentClient::listenSucceeded,
            this, [this](const QString& address, int port) {
        logText(LVL_INFO, QString("Listening on %1:%2").arg(address).arg(port));
    });
    connect(_client, &TorrentClient::listenFailed,
            this, [this](const QString& address, int port, const QString& error) {
        logText(LVL_ERROR, QString("Listen failed on %1:%2 — %3").arg(address).arg(port).arg(error));
    });

    _startedAt = QDateTime::currentDateTimeUtc();
    logText(LVL_INFO, QString("Daemon started; downloads=%1 resume=%2").arg(downloadDir, resumeDir));

    TorrentControlServer::instance()->start();
}

void KanoopTorrentDaemon::threadFinished()
{
    if(TorrentControlServer::instance()->isRunning()) {
        TorrentControlServer::instance()->stop();
    }

    if(_client) {
        _client->saveAllResumeData();
        delete _client;
        _client = nullptr;
    }
    logText(LVL_INFO, "Daemon stopped");
}
