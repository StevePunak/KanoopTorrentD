#ifndef ADMINBODIES_H
#define ADMINBODIES_H

#include <optional>

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <Kanoop/serialization/ideserializablefromjson.h>
#include <Kanoop/serialization/iserializabletojson.h>

class HealthResponseBody : public ISerializableToJson, public ISerializableToJsonObject
{
public:
    HealthResponseBody() = default;
    HealthResponseBody(const QString& status, const QDateTime& startedAt, qint64 uptimeSeconds) :
        _status(status), _startedAt(startedAt), _uptimeSeconds(uptimeSeconds) {}

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;

private:
    QString _status;
    QDateTime _startedAt;
    qint64 _uptimeSeconds = 0;
};

class VersionResponseBody : public ISerializableToJson, public ISerializableToJsonObject
{
public:
    VersionResponseBody() = default;
    VersionResponseBody(const QString& version, const QString& gitSha,
                        const QString& buildTimestamp, const QString& qtVersion) :
        _version(version), _gitSha(gitSha), _buildTimestamp(buildTimestamp), _qtVersion(qtVersion) {}

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;

private:
    QString _version;
    QString _gitSha;
    QString _buildTimestamp;
    QString _qtVersion;
};

/**
 * @brief Settings body used both as the GET response and as a partial PUT request.
 *
 * Only fields that have_value are emitted on serialization, so a partial
 * update body round-trips cleanly. The PUT handler iterates the has* flags
 * to decide which Settings fields to mutate.
 */
class SettingsBody : public ISerializableToJson, public ISerializableToJsonObject,
                     public IDeserializableFromJson, public IDeserializableFromJsonObject
{
public:
    SettingsBody() = default;
    explicit SettingsBody(const QByteArray& json) { deserializeFromJson(json); }

    bool hasDownloadDirectory() const { return _downloadDir.has_value(); }
    QString downloadDirectory() const { return _downloadDir.value_or(QString()); }
    void setDownloadDirectory(const QString& v) { _downloadDir = v; }

    bool hasResumeDataDirectory() const { return _resumeDir.has_value(); }
    QString resumeDataDirectory() const { return _resumeDir.value_or(QString()); }
    void setResumeDataDirectory(const QString& v) { _resumeDir = v; }

    bool hasListenPort() const { return _listenPort.has_value(); }
    int listenPort() const { return _listenPort.value_or(0); }
    void setListenPort(int v) { _listenPort = v; }

    bool hasControlBindAddress() const { return _controlBindAddress.has_value(); }
    QString controlBindAddress() const { return _controlBindAddress.value_or(QString()); }
    void setControlBindAddress(const QString& v) { _controlBindAddress = v; }

    bool hasControlListenPort() const { return _controlListenPort.has_value(); }
    int controlListenPort() const { return _controlListenPort.value_or(0); }
    void setControlListenPort(int v) { _controlListenPort = v; }

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;
    void deserializeFromJson(const QByteArray& json) override;
    void deserializeFromJsonObject(const QJsonObject& object) override;

private:
    std::optional<QString> _downloadDir;
    std::optional<QString> _resumeDir;
    std::optional<int> _listenPort;
    std::optional<QString> _controlBindAddress;
    std::optional<int> _controlListenPort;
};

class SettingsUpdateResultBody : public ISerializableToJson, public ISerializableToJsonObject
{
public:
    SettingsUpdateResultBody() = default;

    void addApplied(const QString& field) { _applied.append(field); }
    void addRequiresRestart(const QString& field) { _requiresRestart.append(field); }
    void addError(const QString& message) { _errors.append(message); }

    QStringList applied() const { return _applied; }
    QStringList requiresRestart() const { return _requiresRestart; }
    QStringList errors() const { return _errors; }

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;

private:
    QStringList _applied;
    QStringList _requiresRestart;
    QStringList _errors;
};

#endif // ADMINBODIES_H
