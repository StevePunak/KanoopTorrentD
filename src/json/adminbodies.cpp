#include "adminbodies.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
const QString KEY_STATUS               = "status";
const QString KEY_STARTED_AT           = "started_at";
const QString KEY_UPTIME_SECONDS       = "uptime_seconds";

const QString KEY_VERSION              = "version";
const QString KEY_GIT_SHA              = "git_sha";
const QString KEY_BUILD_TIMESTAMP      = "build_timestamp";
const QString KEY_QT_VERSION           = "qt_version";

const QString KEY_DOWNLOAD_DIR         = "download_dir";
const QString KEY_RESUME_DIR           = "resume_dir";
const QString KEY_LISTEN_PORT          = "listen_port";
const QString KEY_CONTROL_BIND_ADDRESS = "control_bind_address";
const QString KEY_CONTROL_LISTEN_PORT  = "control_listen_port";

const QString KEY_APPLIED              = "applied";
const QString KEY_REQUIRES_RESTART     = "requires_restart";
const QString KEY_ERRORS               = "errors";

QJsonArray toJsonArray(const QStringList& list)
{
    QJsonArray array;
    for(const QString& item : list) {
        array.append(item);
    }
    return array;
}
} // namespace

// ─── HealthResponseBody ──────────────────────────────────────────────────────

QByteArray HealthResponseBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject HealthResponseBody::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_STATUS] = _status;
    object[KEY_STARTED_AT] = _startedAt.toString(Qt::ISODate);
    object[KEY_UPTIME_SECONDS] = _uptimeSeconds;
    return object;
}

// ─── VersionResponseBody ─────────────────────────────────────────────────────

QByteArray VersionResponseBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject VersionResponseBody::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_VERSION] = _version;
    object[KEY_GIT_SHA] = _gitSha;
    object[KEY_BUILD_TIMESTAMP] = _buildTimestamp;
    object[KEY_QT_VERSION] = _qtVersion;
    return object;
}

// ─── SettingsBody ────────────────────────────────────────────────────────────

QByteArray SettingsBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject SettingsBody::serializeToJsonObject() const
{
    QJsonObject object;
    if(_downloadDir.has_value())        { object[KEY_DOWNLOAD_DIR]         = *_downloadDir; }
    if(_resumeDir.has_value())          { object[KEY_RESUME_DIR]           = *_resumeDir; }
    if(_listenPort.has_value())         { object[KEY_LISTEN_PORT]          = *_listenPort; }
    if(_controlBindAddress.has_value()) { object[KEY_CONTROL_BIND_ADDRESS] = *_controlBindAddress; }
    if(_controlListenPort.has_value())  { object[KEY_CONTROL_LISTEN_PORT]  = *_controlListenPort; }
    return object;
}

void SettingsBody::deserializeFromJson(const QByteArray& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if(doc.isObject()) {
        deserializeFromJsonObject(doc.object());
    }
}

void SettingsBody::deserializeFromJsonObject(const QJsonObject& object)
{
    if(object.contains(KEY_DOWNLOAD_DIR))         { _downloadDir        = object.value(KEY_DOWNLOAD_DIR).toString(); }
    if(object.contains(KEY_RESUME_DIR))           { _resumeDir          = object.value(KEY_RESUME_DIR).toString(); }
    if(object.contains(KEY_LISTEN_PORT))          { _listenPort         = object.value(KEY_LISTEN_PORT).toInt(); }
    if(object.contains(KEY_CONTROL_BIND_ADDRESS)) { _controlBindAddress = object.value(KEY_CONTROL_BIND_ADDRESS).toString(); }
    if(object.contains(KEY_CONTROL_LISTEN_PORT))  { _controlListenPort  = object.value(KEY_CONTROL_LISTEN_PORT).toInt(); }
}

// ─── SettingsUpdateResultBody ────────────────────────────────────────────────

QByteArray SettingsUpdateResultBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject SettingsUpdateResultBody::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_APPLIED] = toJsonArray(_applied);
    object[KEY_REQUIRES_RESTART] = toJsonArray(_requiresRestart);
    object[KEY_ERRORS] = toJsonArray(_errors);
    return object;
}
