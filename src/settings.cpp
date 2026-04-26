#include "settings.h"

#include <QDir>
#include <QMutex>
#include <QStandardPaths>

const QString Settings::KEY_DOWNLOAD_DIR         = "download_dir";
const QString Settings::KEY_RESUME_DIR           = "resume_dir";
const QString Settings::KEY_LISTEN_PORT          = "listen_port";
const QString Settings::KEY_CONTROL_BIND_ADDRESS = "control_bind_address";
const QString Settings::KEY_CONTROL_LISTEN_PORT  = "control_listen_port";

Settings* Settings::instance()
{
    static Settings* _instance = nullptr;
    static QMutex _startLock;

    _startLock.lock();
    if(_instance == nullptr) {
        _instance = new Settings;
        _instance->ensureValidDefaults();
    }
    _startLock.unlock();
    return _instance;
}

Settings::Settings() :
    AppSettings()
{
    setGlobalInstance(this);
}

void Settings::ensureValidDefaults()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    if(downloadDirectory().isEmpty()) {
        setDownloadDirectory(QDir(dataDir).filePath("downloads"));
    }
    if(resumeDataDirectory().isEmpty()) {
        setResumeDataDirectory(QDir(dataDir).filePath("resume"));
    }
}
