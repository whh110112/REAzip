#pragma once
#include <QJsonObject>
#include <QStringList>
class Settings {
 public:
  explicit Settings(QString directory);
  bool load(QString legacyDirectory={});
  bool set(QString key,QJsonValue value);bool save();
  const QJsonObject& values()const{return data;}
  QString directory()const{return root;}QStringList takeNotices(){auto result=notices;notices.clear();return result;}
  static QJsonObject validated(const QJsonObject&);
 private:QString root;QJsonObject data;QStringList notices;bool readOnly{};
};
