#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
class Themes {
 public:
  void load(QString directory,QJsonArray builtins);
  const QJsonArray& all()const{return items;}QStringList notices()const{return warnings;}
  static QString color(QString value);
 private:QJsonArray items;QStringList warnings;
};
