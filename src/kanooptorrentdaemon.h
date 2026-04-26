#ifndef KANOOPTORRENTDAEMON_H
#define KANOOPTORRENTDAEMON_H

#include <Kanoop/utility/abstractthreadclass.h>

class TorrentClient;

class KanoopTorrentDaemon : public AbstractThreadClass
{
    Q_OBJECT
public:
    KanoopTorrentDaemon();

private:
    void threadStarted() override;
    void threadFinished() override;

    TorrentClient* _client = nullptr;
};

#endif // KANOOPTORRENTDAEMON_H
