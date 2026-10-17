#include "EntryModel.hpp"
#include <QFileInfo>
#include <QCollator>
#include <algorithm>
QHash<int,QByteArray> EntryModel::roleNames()const{return {{Id,"entryId"},{Name,"name"},{Directory,"directory"},{Size,"displaySize"},{Packed,"displayPacked"},{Kind,"kind"},{Modified,"modified"},{Selected,"selected"}};}
QString EntryModel::bytes(quint64 value){const char* units[]={"B","KB","MB","GB","TB"};int i=0;double n=value;while(n>=1024&&i<4){n/=1024;++i;}return QString::number(n,'f',i?1:0)+" "+units[i];}
QVariant EntryModel::data(const QModelIndex& index,int role)const{
  if(!index.isValid()||index.row()<0||index.row()>=rows.size())return {};
  const auto& item=rows[index.row()];const auto id=item["entryId"].toString();const bool dir=item["directory"].toBool();
  switch(role){case Id:return id;case Name:return item["name"].toString();case Directory:return dir;case Selected:return selected.contains(id);
  case Size:return dir?QString::fromUtf8("—"):bytes(item["size"].toString().toULongLong());
  case Packed:return dir||!item.contains("packedSize")?QString::fromUtf8("—"):bytes(item["packedSize"].toString().toULongLong());
  case Kind:return dir?"DIR":QFileInfo(item["name"].toString()).suffix().left(5).toUpper();
  case Modified:return item["modified"].toString(QString::fromUtf8("—"));default:return {};}
}
void EntryModel::load(QJsonArray entries,QString rev,QString folder){source=std::move(entries);revision=std::move(rev);currentFolder=std::move(folder);selected.clear();rebuild();}
void EntryModel::filter(QString value){query=std::move(value);selected.clear();rebuild();}
void EntryModel::sort(int value){ascending=column==value?!ascending:true;column=value;rebuild();}
void EntryModel::select(QString id,bool additive){if(!additive)selected.clear();if(selected.contains(id))selected.remove(id);else selected.insert(id);if(!rows.isEmpty())emit dataChanged(index(0),index(rows.size()-1),{Selected});emit changed();}
QString EntryModel::selectedName()const{if(selected.size()!=1)return {};return entry(*selected.begin())["name"].toString();}
QJsonObject EntryModel::entry(QString id)const{for(const auto& row:rows)if(row["entryId"]==id)return row;return {};}
bool EntryModel::enter(QString id){const auto item=entry(id);if(!item["directory"].toBool())return false;currentFolder=item["path"].toString();query.clear();selected.clear();rebuild();return true;}
void EntryModel::parentFolder(){const auto index=currentFolder.lastIndexOf('/');currentFolder=index<0?QString():currentFolder.left(index);query.clear();selected.clear();rebuild();}
void EntryModel::rebuild(){
  beginResetModel();rows.clear();QHash<QString,int> directories;const auto prefix=currentFolder.isEmpty()?QString():currentFolder+'/';
  for(const auto& value:source){auto item=value.toObject();auto path=item["path"].toString();while(path.endsWith('/'))path.chop(1);if(!path.startsWith(prefix))continue;auto relative=path.mid(prefix.size());if(relative.isEmpty())continue;
    if(!query.isEmpty()){if(!relative.contains(query,Qt::CaseInsensitive))continue;item["name"]=relative;rows.append(item);continue;}
    const auto slash=relative.indexOf('/');if(slash>=0){const auto name=relative.left(slash);if(!directories.contains(name)){directories[name]=rows.size();rows.append(QJsonObject{{"entryId",revision+":dir:"+prefix+name},{"name",name},{"path",prefix+name},{"directory",true}});}continue;}
    item["name"]=relative;if(item["directory"].toBool()&&directories.contains(relative))rows[directories[relative]]=item;else{if(item["directory"].toBool())directories[relative]=rows.size();rows.append(item);}
  }
  QCollator collator;collator.setNumericMode(true);collator.setCaseSensitivity(Qt::CaseInsensitive);
  std::stable_sort(rows.begin(),rows.end(),[&](const QJsonObject& a,const QJsonObject& b){if(a["directory"].toBool()!=b["directory"].toBool())return a["directory"].toBool();int result{};
    if(column==1||column==2){const auto key=column==1?"size":"packedSize";const auto x=a[key].toString().toULongLong(),y=b[key].toString().toULongLong();result=x<y?-1:x>y?1:0;}
    else result=collator.compare(a[column==3?"encryption":column==4?"modified":"name"].toString(),b[column==3?"encryption":column==4?"modified":"name"].toString());
    return ascending?result<0:result>0;});
  endResetModel();emit changed();
}
