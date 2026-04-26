#ifndef TORRENTCONTROLSERVER_H
#define TORRENTCONTROLSERVER_H

#include <Kanoop/utility/abstractthreadclass.h>

class QHttpServer;
class QHttpServerRequest;
class QHttpServerResponse;
class QTcpServer;

class TorrentControlServer : public AbstractThreadClass
{
    Q_OBJECT
public:
    static TorrentControlServer* instance();

private:
    TorrentControlServer();

    void threadStarted() override;
    void threadFinished() override;

    QHttpServerResponse handleHealth(const QHttpServerRequest& request);
    QHttpServerResponse handleVersion(const QHttpServerRequest& request);
    QHttpServerResponse handleSettingsGet(const QHttpServerRequest& request);
    QHttpServerResponse handleSettingsPut(const QHttpServerRequest& request);
    QHttpServerResponse handleSearch(const QHttpServerRequest& request);
    QHttpServerResponse handleAddTorrent(const QHttpServerRequest& request);

    QHttpServer* _httpServer = nullptr;
    QTcpServer* _tcpServer = nullptr;
};

#endif // TORRENTCONTROLSERVER_H
