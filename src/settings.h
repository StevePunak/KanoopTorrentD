#ifndef SETTINGS_H
#define SETTINGS_H

#include <Kanoop/utility/appsettings.h>

class Settings : public AppSettings
{
    Q_OBJECT

public:
    static Settings* instance();

    Settings();

    QString downloadDirectory() const { return _settings.value(KEY_DOWNLOAD_DIR).toString(); }
    void setDownloadDirectory(const QString& value) { _settings.setValue(KEY_DOWNLOAD_DIR, value); }

    QString resumeDataDirectory() const { return _settings.value(KEY_RESUME_DIR).toString(); }
    void setResumeDataDirectory(const QString& value) { _settings.setValue(KEY_RESUME_DIR, value); }

    int listenPort() const { return _settings.value(KEY_LISTEN_PORT, 6881).toInt(); }
    void setListenPort(int value) { _settings.setValue(KEY_LISTEN_PORT, value); }

    QString controlBindAddress() const { return _settings.value(KEY_CONTROL_BIND_ADDRESS, "127.0.0.1").toString(); }
    void setControlBindAddress(const QString& value) { _settings.setValue(KEY_CONTROL_BIND_ADDRESS, value); }

    int controlListenPort() const { return _settings.value(KEY_CONTROL_LISTEN_PORT, 8901).toInt(); }
    void setControlListenPort(int value) { _settings.setValue(KEY_CONTROL_LISTEN_PORT, value); }

private:
    void ensureValidDefaults() override;

    static const QString KEY_DOWNLOAD_DIR;
    static const QString KEY_RESUME_DIR;
    static const QString KEY_LISTEN_PORT;
    static const QString KEY_CONTROL_BIND_ADDRESS;
    static const QString KEY_CONTROL_LISTEN_PORT;
};

#endif // SETTINGS_H
