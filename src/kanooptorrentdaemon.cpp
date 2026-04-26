#include "kanooptorrentdaemon.h"

#include <QDir>

#include <Kanoop/torrent/torrentclient.h>

#include "settings.h"

KanoopTorrentDaemon::KanoopTorrentDaemon() :
    AbstractThreadClass("torrent-daemon")
{
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

    logText(LVL_INFO, QString("Daemon started; downloads=%1 resume=%2").arg(downloadDir, resumeDir));
}

void KanoopTorrentDaemon::threadFinished()
{
    if(_client) {
        _client->saveAllResumeData();
        delete _client;
        _client = nullptr;
    }
    logText(LVL_INFO, "Daemon stopped");
}
