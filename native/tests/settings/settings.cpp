#include "../../settings/Settings.hpp"
#include "../../themes/Themes.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);QTemporaryDir work;const auto legacy=work.path()+"/legacy",data=work.path()+"/native";QDir().mkpath(legacy+"/skins");
  auto write=[](QString path,QByteArray bytes){QFile f(path);if(!f.open(QIODevice::WriteOnly))return false;return f.write(bytes)==bytes.size();};
  auto read=[](QString path){QFile f(path);f.open(QIODevice::ReadOnly);return f.readAll();};
  const auto old=QJsonObject{{"skin","custom"},{"language","ar"},{"codepage","936"},{"restoreTabs",false},{"openTabs",QJsonArray{work.path()+"/missing.zip"}},{"password","private-test-value"},{"unsupported","ignored"}};
  const auto original=QJsonDocument(old).toJson();write(legacy+"/settings.json",original);write(legacy+"/skins/custom.json",R"json({"id":"custom","name":"Custom","base":"dark","vars":{"--accent":"rgba(1,2,3,0.5)","--text":"rgb(100,120,140)","--unknown":"bad"},"css":"body { color: red; }"})json");
  Settings first(data);if(!first.load(legacy)||first.values()["language"]!="ar"||first.values().contains("password"))return 1;if(read(legacy+"/settings.json")!=original)return 2;
  for(const auto& name:QDir(data).entryList({"*.json"},QDir::Files))if(read(data+'/'+name).contains("private-test-value"))return 3;
  if(first.set("language","invalid")||first.set("restoreTabs","yes")||first.set("password","private-test-value")||first.set("codepage","9999"))return 4;
  if(!first.set("language","en"))return 5;Settings second(data);if(!second.load(legacy)||second.values()["language"]!="en")return 6;
  write(data+"/settings.json","{invalid");Settings damaged(data);if(!damaged.load()||QDir(data).entryList({"*.corrupt-*"},QDir::Files).size()!=1)return 7;
  write(data+"/settings.json",R"({"schemaVersion":99,"language":"ja"})");Settings future(data);if(future.load()||future.set("language","en")||!read(data+"/settings.json").contains("99"))return 8;
  const QJsonArray builtins{QJsonObject{{"id","light"},{"vars",QJsonObject{{"--accent","#123456"},{"--text","#000000"}}}},QJsonObject{{"id","sleep"},{"vars",QJsonObject{{"--accent","#abcdef"},{"--text","#ffffff"}}}}};Themes themes;themes.load(data+"/themes",builtins);if(themes.all().size()!=3||themes.notices().size()!=2)return 9;
  const auto imported=themes.all().last().toObject();if(imported.contains("css")||imported["vars"].toObject()["--accent"]!="#80010203"||!Themes::color("rgba(999,0,0,.5)").isEmpty()||Themes::color("rgb(100,120,140)")!="#ff64788c")return 10;
  std::cout<<"validated atomic settings, secret whitelist, migration idempotence, damaged/future schema, CSS/token conversion PASS\n";return 0;
}
