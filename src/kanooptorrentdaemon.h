#ifndef KANOOPTORRENTDAEMON_H
#define KANOOPTORRENTDAEMON_H

#include <QDateTime>
#include <QJsonObject>

#include <Kanoop/utility/abstractthreadclass.h>

class TorrentClient;

class KanoopTorrentDaemon : public AbstractThreadClass
{
    Q_OBJECT
public:
    static KanoopTorrentDaemon* instance();

    Q_INVOKABLE QDateTime startedAt() const { return _startedAt; }
    Q_INVOKABLE qint64 uptimeSeconds() const;

    Q_INVOKABLE QJsonObject addMagnet(const QString& magnetUri);

private:
    KanoopTorrentDaemon();

    void threadStarted() override;
    void threadFinished() override;

    TorrentClient* _client = nullptr;
    QDateTime _startedAt;
};

#endif // KANOOPTORRENTDAEMON_H
