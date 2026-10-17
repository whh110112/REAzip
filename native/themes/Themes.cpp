#include "Themes.hpp"
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
QString Themes::color(QString value){value=value.trimmed();QColor native(value);if(native.isValid())return native.name(QColor::HexArgb);const auto match=QRegularExpression("^rgba?\\(\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*(?:,\\s*([0-9.]+)\\s*)?\\)$",QRegularExpression::CaseInsensitiveOption).match(value);if(!match.hasMatch())return {};const auto red=match.captured(1).toDouble(),green=match.captured(2).toDouble(),blue=match.captured(3).toDouble(),alpha=match.captured(4).isEmpty()?1.0:match.captured(4).toDouble();if(red>255||green>255||blue>255||alpha>1||alpha<0)return {};return QColor::fromRgbF(red/255,green/255,blue/255,alpha).name(QColor::HexArgb);}
void Themes::load(QString directory,QJsonArray builtins){items={};warnings.clear();QSet<QString> ids,allowed;
  for(const auto& value:builtins){auto item=value.toObject();auto vars=item["vars"].toObject();for(auto it=vars.begin();it!=vars.end();){auto converted=color(it.value().toString());if(converted.isEmpty())it=vars.erase(it);else{allowed.insert(it.key());it.value()=converted;++it;}}item["vars"]=vars;ids.insert(item["id"].toString());items.append(item);}
  for(const auto& name:QDir(directory).entryList({"*.json"},QDir::Files)){QFile file(directory+'/'+name);if(QFileInfo(file).isSymLink()||!file.open(QIODevice::ReadOnly)||file.size()>256*1024){warnings<<name+": unreadable or oversized theme";continue;}const auto doc=QJsonDocument::fromJson(file.readAll());const auto input=doc.object();const auto id=input["id"].toString();if(!doc.isObject()||!QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(id).hasMatch()||ids.contains(id)||!input["vars"].isObject()){warnings<<name+": invalid/duplicate theme";continue;}
    QJsonObject item{{"id",id},{"name",input["name"].toString(id).left(128)},{"base",input["base"].toString()=="dark"?"dark":"light"},{"author",input["author"].toString().left(128)}};
    QJsonObject vars;for(const auto& builtin:items){if(builtin.toObject()["id"].toString()==(item["base"]=="dark"?"sleep":"light")){vars=builtin.toObject()["vars"].toObject();break;}}
    const auto overrides=input["vars"].toObject();for(auto it=overrides.begin();it!=overrides.end();++it){auto converted=color(it.value().toString());if(allowed.contains(it.key())&&!converted.isEmpty())vars[it.key()]=converted;else warnings<<name+": ignored token "+it.key();}
    if(!input["css"].toString().isEmpty()||!input["extraCss"].toString().isEmpty())warnings<<name+": custom CSS preserved in original JSON but cannot execute in Qt Quick";
    item["vars"]=vars;items.append(item);ids.insert(id);
  }
}
