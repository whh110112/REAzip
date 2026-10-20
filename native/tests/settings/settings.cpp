#include "../../settings/Settings.hpp"
#include "../../themes/Themes.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImage>
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);QTemporaryDir work;const auto legacy=work.path()+"/legacy",data=work.path()+"/native";QDir().mkpath(legacy+"/skins");
  auto write=[](QString path,QByteArray bytes){QFile f(path);if(!f.open(QIODevice::WriteOnly))return false;return f.write(bytes)==bytes.size();};
  auto read=[](QString path){QFile f(path);f.open(QIODevice::ReadOnly);return f.readAll();};
  const auto old=QJsonObject{{"skin","custom"},{"language","ar"},{"codepage","936"},{"restoreTabs",false},{"openTabs",QJsonArray{work.path()+"/missing.zip"}},{"password","private-test-value"},{"unsupported","ignored"}};
  const auto original=QJsonDocument(old).toJson();write(legacy+"/settings.json",original);write(legacy+"/skins/custom.json",R"json({"id":"custom","name":"Custom","base":"dark","vars":{"--accent":"rgba(1,2,3,0.5)","--text":"rgb(100,120,140)","--unknown":"bad"},"css":"body { color: red; }"})json");
  Settings first(data);if(!first.load(legacy)||first.values()["language"]!="ar"||first.values().contains("password"))return 1;if(read(legacy+"/settings.json")!=original)return 2;
  for(const auto& name:QDir(data).entryList({"*.json"},QDir::Files))if(read(data+'/'+name).contains("private-test-value"))return 3;
  if(first.set("language","invalid")||first.set("restoreTabs","yes")||first.set("password","private-test-value")||first.set("codepage","9999"))return 4;
  if(Settings::validated({})["extractDirMode"]!="archive")return 14;
  const auto downloads=QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  if(Settings::validated({{"extractDir",downloads}})["extractDirMode"]!="archive")return 15;
  if(Settings::validated({{"extractDir",work.path()}})["extractDirMode"]!="custom")return 16;
  if(first.set("extractDirMode","invalid")||!first.set("extractDirMode","custom")||!first.set("extractDir",work.path()))return 17;
  {Settings saved(data);if(!saved.load()||saved.values()["extractDirMode"]!="custom"||saved.values()["extractDir"]!=work.path())return 18;}
  if(!first.set("extractDirMode","archive")||first.values()["extractDir"]!=work.path())return 19;
  if(!first.set("language","en"))return 5;Settings second(data);if(!second.load(legacy)||second.values()["language"]!="en")return 6;
  write(data+"/settings.json","{invalid");Settings damaged(data);if(!damaged.load()||QDir(data).entryList({"*.corrupt-*"},QDir::Files).size()!=1)return 7;
  write(data+"/settings.json",R"({"schemaVersion":99,"language":"ja"})");Settings future(data);if(future.load()||future.set("language","en")||!read(data+"/settings.json").contains("99"))return 8;
  const QJsonArray builtins{QJsonObject{{"id","light"},{"vars",QJsonObject{{"--accent","#123456"},{"--text","#000000"}}}},QJsonObject{{"id","sleep"},{"vars",QJsonObject{{"--accent","#abcdef"},{"--text","#ffffff"}}}}};Themes themes;themes.load(data+"/themes",builtins);if(themes.all().size()!=3||themes.notices().size()!=2){std::cerr<<"Theme import notices: "<<themes.notices().join("; ").toStdString()<<'\n';return 9;}
  const auto imported=themes.all().last().toObject();if(imported.contains("css")||imported["vars"].toObject()["--accent"]!="#80010203"||!Themes::color("rgba(999,0,0,.5)").isEmpty()||Themes::color("rgb(100,120,140)")!="#ff64788c")return 10;
  const auto themeDir=data+"/themes";QDir().mkpath(themeDir+"/textures");
  QImage texture(8,8,QImage::Format_RGB32);texture.fill(Qt::gray);if(!texture.save(themeDir+"/textures/paper.png"))return 11;
  texture.save(data+"/outside.png");
  write(themeDir+"/pattern.json",R"json({"id":"pattern","base":"dark","vars":{"--accent":"#999999"},"background":{"pattern":"grid","opacity":0.12,"spacing":12,"texture":"textures/paper.png"}})json");
  Themes patterned;patterned.load(themeDir,builtins);QJsonObject appearance;
  for(const auto& item:patterned.all())if(item.toObject()["id"]=="pattern")appearance=item.toObject()["background"].toObject();
  if(appearance["pattern"]!="grid"||appearance["opacity"].toDouble()!=0.12||appearance["spacing"].toInt()!=12||QUrl(appearance["texture"].toString()).toLocalFile()!=QFileInfo(themeDir+"/textures/paper.png").canonicalFilePath())return 12;
  for(const auto& unsafe:{QString("../outside.png"),QString("https://example.invalid/image.png"),data+"/outside.png"}){
    write(themeDir+"/unsafe.json",QJsonDocument(QJsonObject{{"id","unsafe"},{"vars",QJsonObject{}},{"background",QJsonObject{{"pattern","unknown"},{"spacing",0},{"opacity",9},{"texture",unsafe}}}}).toJson());
    Themes guarded;guarded.load(themeDir,builtins);
    for(const auto& item:guarded.all())if(item.toObject()["id"]=="unsafe"){
      const auto bg=item.toObject()["background"].toObject();if(bg["pattern"]!="none"||bg["spacing"].toInt()!=16||bg["opacity"].toDouble()!=0.1||!bg["texture"].toString().isEmpty())return 13;
    }
  }
  std::cout<<"validated atomic settings, secret whitelist, migration idempotence, damaged/future schema, CSS/token conversion PASS\n";return 0;
}
