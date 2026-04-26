#include "torrentbodies.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <Kanoop/torrent/magnetlink.h>
#include <Kanoop/torrent/torrentsearchresult.h>

namespace {
const QString KEY_NAME           = "name";
const QString KEY_INFO_HASH      = "info_hash";
const QString KEY_SIZE           = "size";
const QString KEY_SIZE_HUMAN     = "size_human";
const QString KEY_SEEDERS        = "seeders";
const QString KEY_LEECHERS       = "leechers";
const QString KEY_ADDED_DATE     = "added_date";
const QString KEY_CATEGORY       = "category";
const QString KEY_UPLOADER_NAME  = "uploader_name";
const QString KEY_MAGNET         = "magnet";

const QString KEY_QUERY          = "query";
const QString KEY_RESULTS        = "results";

const QString KEY_DISPLAY_NAME   = "display_name";
} // namespace

// ─── SearchResultRow ─────────────────────────────────────────────────────────

SearchResultRow::SearchResultRow(const TorrentSearchResult& source) :
    _name(source.name()),
    _infoHash(source.infoHash()),
    _size(source.size()),
    _sizeHuman(TorrentSearchResult::formatSize(source.size())),
    _seeders(source.seeders()),
    _leechers(source.leechers()),
    _addedDate(source.addedDate().toString(Qt::ISODate)),
    _category(source.category()),
    _uploaderName(source.uploaderName()),
    _magnet(source.toMagnetLink().toUri())
{
}

QJsonObject SearchResultRow::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_NAME]          = _name;
    object[KEY_INFO_HASH]     = _infoHash;
    object[KEY_SIZE]          = _size;
    object[KEY_SIZE_HUMAN]    = _sizeHuman;
    object[KEY_SEEDERS]       = _seeders;
    object[KEY_LEECHERS]      = _leechers;
    object[KEY_ADDED_DATE]    = _addedDate;
    object[KEY_CATEGORY]      = _category;
    object[KEY_UPLOADER_NAME] = _uploaderName;
    object[KEY_MAGNET]        = _magnet;
    return object;
}

// ─── SearchResponseBody ──────────────────────────────────────────────────────

QByteArray SearchResponseBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject SearchResponseBody::serializeToJsonObject() const
{
    QJsonArray array;
    for(const SearchResultRow& row : _results) {
        array.append(row.serializeToJsonObject());
    }
    QJsonObject object;
    object[KEY_QUERY]   = _query;
    object[KEY_RESULTS] = array;
    return object;
}

// ─── AddTorrentRequestBody ───────────────────────────────────────────────────

void AddTorrentRequestBody::deserializeFromJson(const QByteArray& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if(doc.isObject()) {
        deserializeFromJsonObject(doc.object());
    }
}

void AddTorrentRequestBody::deserializeFromJsonObject(const QJsonObject& object)
{
    _magnet = object.value(KEY_MAGNET).toString();
}

// ─── AddTorrentResponseBody ──────────────────────────────────────────────────

QByteArray AddTorrentResponseBody::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject AddTorrentResponseBody::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_INFO_HASH]    = _infoHash;
    object[KEY_DISPLAY_NAME] = _displayName;
    return object;
}
