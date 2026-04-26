#include "operationresult.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace { const QString KEY_SUCCESS = "success"; const QString KEY_MESSAGE = "message"; }

QByteArray OperationResult::serializeToJson() const
{
    return QJsonDocument(serializeToJsonObject()).toJson(QJsonDocument::Compact);
}

QJsonObject OperationResult::serializeToJsonObject() const
{
    QJsonObject object;
    object[KEY_SUCCESS] = _success;
    object[KEY_MESSAGE] = _message;
    return object;
}

void OperationResult::deserializeFromJson(const QByteArray& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if(doc.isObject()) {
        deserializeFromJsonObject(doc.object());
    }
}

void OperationResult::deserializeFromJsonObject(const QJsonObject& object)
{
    _success = object.value(KEY_SUCCESS).toBool();
    _message = object.value(KEY_MESSAGE).toString();
}
