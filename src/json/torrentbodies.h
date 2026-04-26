#ifndef TORRENTBODIES_H
#define TORRENTBODIES_H

#include <QList>
#include <QString>

#include <Kanoop/serialization/ideserializablefromjson.h>
#include <Kanoop/serialization/iserializabletojson.h>

class TorrentSearchResult;

class SearchResultRow : public ISerializableToJsonObject
{
public:
    SearchResultRow() = default;
    explicit SearchResultRow(const TorrentSearchResult& source);

    QJsonObject serializeToJsonObject() const override;

private:
    QString _name;
    QString _infoHash;
    qint64  _size = 0;
    QString _sizeHuman;
    int     _seeders = 0;
    int     _leechers = 0;
    QString _addedDate;
    QString _category;
    QString _uploaderName;
    QString _magnet;
};

class SearchResponseBody : public ISerializableToJson, public ISerializableToJsonObject
{
public:
    SearchResponseBody() = default;
    SearchResponseBody(const QString& query, const QList<SearchResultRow>& results) :
        _query(query), _results(results) {}

    void setQuery(const QString& v) { _query = v; }
    void appendResult(const SearchResultRow& row) { _results.append(row); }

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;

private:
    QString _query;
    QList<SearchResultRow> _results;
};

class AddTorrentRequestBody : public IDeserializableFromJson, public IDeserializableFromJsonObject
{
public:
    AddTorrentRequestBody() = default;
    explicit AddTorrentRequestBody(const QByteArray& json) { deserializeFromJson(json); }

    QString magnet() const { return _magnet; }

    void deserializeFromJson(const QByteArray& json) override;
    void deserializeFromJsonObject(const QJsonObject& object) override;

private:
    QString _magnet;
};

class AddTorrentResponseBody : public ISerializableToJson, public ISerializableToJsonObject
{
public:
    AddTorrentResponseBody() = default;
    AddTorrentResponseBody(const QString& infoHash, const QString& displayName) :
        _infoHash(infoHash), _displayName(displayName) {}

    QString infoHash() const { return _infoHash; }
    QString displayName() const { return _displayName; }

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;

private:
    QString _infoHash;
    QString _displayName;
};

#endif // TORRENTBODIES_H
