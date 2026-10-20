#include "Themes.hpp"
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QImageReader>
#include <QDateTime>
#include <QUrl>
#include <cmath>
namespace {
QJsonObject background(const QJsonValue& value, const QString& directory, QStringList& warnings, const QString& name) {
  QJsonObject result{{"pattern","none"},{"opacity",0.1},{"spacing",16},{"texture",""}};
  if (value.isUndefined()) return result;
  if (!value.isObject()) { warnings << name+": invalid background"; return result; }
  const auto input=value.toObject();
  const auto pattern=input["pattern"].toString("none");
  if (QStringList{"none","dots","lines","grid"}.contains(pattern)) result["pattern"]=pattern;
  else warnings << name+": unsupported background pattern";
  for (const auto& key : {QString("opacity"),QString("spacing")}) {
    if (!input.contains(key)) continue;
    const auto number=input[key].toDouble(-1);
    const auto low=key=="opacity"?0.0:8.0, high=key=="opacity"?0.18:64.0;
    if (input[key].isDouble() && std::isfinite(number) && number>=low && number<=high) result[key]=number;
    else warnings << name+": invalid background "+key;
  }
  const auto texture=input["texture"].toString();
  if (!texture.isEmpty()) {
    const auto root=QDir(directory).canonicalPath();
    QFileInfo file(QDir(directory).filePath(texture));
    const auto resolved=file.canonicalFilePath();
    const auto inside=!root.isEmpty() && resolved.startsWith(root+'/',Qt::CaseSensitive);
    const auto extension=file.suffix().toLower();
    bool readable=false;
    if (!QDir::isAbsolutePath(texture) && inside && !file.isSymLink() && file.isFile() && file.size()<=4*1024*1024 &&
        QStringList{"png","jpg","jpeg","webp"}.contains(extension)) {
      QImageReader reader(resolved);const auto size=reader.size();
      readable=reader.canRead() && size.width()>0 && size.height()>0 && size.width()<=4096 && size.height()<=4096;
    }
    if (readable) {
      auto url=QUrl::fromLocalFile(resolved);url.setQuery("v="+QString::number(file.lastModified().toMSecsSinceEpoch())+'-'+QString::number(file.size()));
      result["texture"]=url.toString();
    } else warnings << name+": ignored background texture outside theme directory, unsupported, or oversized";
  }
  return result;
}
}
QString Themes::color(QString value){value=value.trimmed();QColor native(value);if(native.isValid())return native.name(QColor::HexArgb);const auto match=QRegularExpression("^rgba?\\(\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*(?:,\\s*([0-9.]+)\\s*)?\\)$",QRegularExpression::CaseInsensitiveOption).match(value);if(!match.hasMatch())return {};const auto red=match.captured(1).toDouble(),green=match.captured(2).toDouble(),blue=match.captured(3).toDouble(),alpha=match.captured(4).isEmpty()?1.0:match.captured(4).toDouble();if(red>255||green>255||blue>255||alpha>1||alpha<0)return {};return QColor::fromRgbF(red/255,green/255,blue/255,alpha).name(QColor::HexArgb);}
void Themes::load(QString directory,QJsonArray builtins){items={};warnings.clear();QSet<QString> ids,allowed;
  for(const auto& value:builtins){auto item=value.toObject();auto vars=item["vars"].toObject();for(auto it=vars.begin();it!=vars.end();){auto converted=color(it.value().toString());if(converted.isEmpty())it=vars.erase(it);else{allowed.insert(it.key());it.value()=converted;++it;}}item["vars"]=vars;item["background"]=background(item.value("background"),directory,warnings,item["id"].toString());ids.insert(item["id"].toString());items.append(item);}
  for(const auto& name:QDir(directory).entryList({"*.json"},QDir::Files)){QFile file(directory+'/'+name);if(QFileInfo(file).isSymLink()||!file.open(QIODevice::ReadOnly)||file.size()>256*1024){warnings<<name+": unreadable or oversized theme";continue;}const auto doc=QJsonDocument::fromJson(file.readAll());const auto input=doc.object();const auto id=input["id"].toString();if(!doc.isObject()||!QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(id).hasMatch()||ids.contains(id)||!input["vars"].isObject()){warnings<<name+": invalid/duplicate theme";continue;}
    QJsonObject item{{"id",id},{"name",input["name"].toString(id).left(128)},{"nameEn",input["nameEn"].toString(input["name"].toString(id)).left(128)},{"base",input["base"].toString()=="dark"?"dark":"light"},{"author",input["author"].toString().left(128)}};
    QJsonObject vars;for(const auto& builtin:items){if(builtin.toObject()["id"].toString()==(item["base"]=="dark"?"sleep":"light")){vars=builtin.toObject()["vars"].toObject();break;}}
    const auto overrides=input["vars"].toObject();for(auto it=overrides.begin();it!=overrides.end();++it){auto converted=color(it.value().toString());if(allowed.contains(it.key())&&!converted.isEmpty())vars[it.key()]=converted;else warnings<<name+": ignored token "+it.key();}
    if(!input["css"].toString().isEmpty()||!input["extraCss"].toString().isEmpty())warnings<<name+": custom CSS preserved in original JSON but cannot execute in Qt Quick";
    item["background"]=background(input["background"],directory,warnings,name);
    item["vars"]=vars;items.append(item);ids.insert(id);
  }
}
