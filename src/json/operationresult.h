#ifndef OPERATIONRESULT_H
#define OPERATIONRESULT_H

#include <QString>

#include <Kanoop/serialization/ideserializablefromjson.h>
#include <Kanoop/serialization/iserializabletojson.h>

class OperationResult : public ISerializableToJson,
                        public ISerializableToJsonObject,
                        public IDeserializableFromJson,
                        public IDeserializableFromJsonObject
{
public:
    OperationResult() = default;
    OperationResult(bool success, const QString& message = QString()) :
        _success(success), _message(message) {}
    explicit OperationResult(const QByteArray& json) { deserializeFromJson(json); }

    bool success() const { return _success; }
    QString message() const { return _message; }

    void setSuccess(bool value) { _success = value; }
    void setMessage(const QString& value) { _message = value; }

    QByteArray serializeToJson() const override;
    QJsonObject serializeToJsonObject() const override;
    void deserializeFromJson(const QByteArray& json) override;
    void deserializeFromJsonObject(const QJsonObject& object) override;

private:
    bool _success = false;
    QString _message;
};

#endif // OPERATIONRESULT_H
