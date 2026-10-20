#include "Settings.hpp"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSet>
namespace {
QJsonObject read(QString path,bool& ok){QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024){ok=false;return {};}QJsonParseError error;auto doc=QJsonDocument::fromJson(file.readAll(),&error);ok=error.error==QJsonParseError::NoError&&doc.isObject();return doc.object();}
bool write(QString path,QJsonObject value){QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return false;file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);const auto bytes=QJsonDocument(value).toJson();return file.write(bytes)==bytes.size()&&file.commit();}
}
Settings::Settings(QString directory):root(std::move(directory)){}
QJsonObject Settings::validated(const QJsonObject& input){
  const auto downloads=QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QJsonObject result{{"schemaVersion",1},{"skin","light"},{"language","zh-CN"},{"restoreTabs",true},{"codepage",""},{"extractDirMode","archive"},{"extractDir",downloads},{"archiveDir",downloads},{"openTabs",QJsonArray{}}};
  if(input["skin"].isString()&&QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(input["skin"].toString()).hasMatch())result["skin"]=input["skin"];
  const QStringList languages={"zh-CN","zh-TW","en","ja","ko","fr","de","es","pt-BR","ru","it","pl","tr","vi","th","id","ar","hi"};if(languages.contains(input["language"].toString()))result["language"]=input["language"];
  if(input["restoreTabs"].isBool())result["restoreTabs"]=input["restoreTabs"];
  if(input["codepage"].isString()&&QStringList{"","936","950","932","65001"}.contains(input["codepage"].toString()))result["codepage"]=input["codepage"];
  for(const auto& key:{"extractDir","archiveDir"})if(input[key].isString()&&QFileInfo(input[key].toString()).isAbsolute()&&!input[key].toString().contains(QChar::Null))result[key]=input[key];
  if(QStringList{"archive","custom"}.contains(input["extractDirMode"].toString()))result["extractDirMode"]=input["extractDirMode"];
  else if(!input.contains("extractDirMode")&&result["extractDir"].toString()!=downloads)result["extractDirMode"]="custom";
  QJsonArray tabs;QSet<QString> seen;for(const auto& value:input["openTabs"].toArray()){const auto path=value.toString();if(value.isString()&&QFileInfo(path).isAbsolute()&&!path.contains(QChar::Null)&&!seen.contains(path)&&tabs.size()<128){seen.insert(path);tabs.append(path);}}result["openTabs"]=tabs;
  if(input["migratedLegacy"].isBool())result["migratedLegacy"]=input["migratedLegacy"];
  return result;
}
bool Settings::load(QString legacy){
  if(!QDir().mkpath(root)){readOnly=true;data=validated({});notices<<"Settings directory is not writable: "+root;return false;}
  bool ok=false;const auto file=root+"/settings.json";
  if(QFileInfo::exists(file)){const auto input=read(file,ok);if(ok){if(input["schemaVersion"].toInt(1)>1){readOnly=true;data=validated(input);notices<<"Configuration is from a newer version; writing is disabled";return false;}data=validated(input);return true;}
    const auto backup=file+".corrupt-"+QString::number(QDateTime::currentMSecsSinceEpoch());if(!QFile::rename(file,backup)){readOnly=true;data=validated({});notices<<"Cannot preserve damaged settings; writing is disabled";return false;}notices<<"Damaged settings preserved at "+backup;
  }
  data=validated({});if(!legacy.isEmpty()&&QFileInfo::exists(legacy+"/settings.json")){
    const auto old=read(legacy+"/settings.json",ok);if(ok){data=validated(old);data["migratedLegacy"]=true;const auto backup=root+"/legacy-settings.sanitized.json";if(!QFileInfo::exists(backup)&&!write(backup,data)){notices<<"Cannot preserve sanitized migration backup";return false;}notices<<"Legacy settings imported; only validated fields copied, original unchanged";}
    else notices<<"Legacy settings are damaged; original left unchanged";
    const auto source=legacy+"/skins";QDir().mkpath(root+"/themes");QDir().mkpath(root+"/legacy-themes");for(const auto& name:QDir(source).entryList({"*.json"},QDir::Files)){const auto from=source+'/'+name;if(QFileInfo(from).isSymLink()||QFileInfo(from).size()>256*1024)continue;const auto target=root+"/themes/"+name;if(!QFileInfo::exists(target))QFile::copy(from,target);const auto backup=root+"/legacy-themes/"+name;if(!QFileInfo::exists(backup))QFile::copy(from,backup);}
  }
  return save();
}
bool Settings::set(QString key,QJsonValue value){if(!validated({}).contains(key)||key=="schemaVersion")return false;auto candidate=data;candidate[key]=value;const auto checked=validated(candidate);if(checked[key]!=value)return false;const auto previous=data;data=checked;if(!save()){data=previous;return false;}return true;}
bool Settings::save(){if(readOnly)return false;if(!write(root+"/settings.json",data)){notices<<"Cannot write settings: "+root;return false;}return true;}
