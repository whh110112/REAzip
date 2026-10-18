#include "Vault.hpp"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QUuid>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif
namespace {
constexpr int kMaxEntries=128;
constexpr qint64 kMaxFileBytes=1024*1024;
QString defaultMode(){
#ifdef Q_OS_WIN
  return "dpapi";
#else
  return "permissions";
#endif
}
QString now(){return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);}
bool text(const QString& value,int limit){if(value.isEmpty()||value.size()>limit)return false;for(const auto c:value)if(c.unicode()<0x20||c.unicode()==0x7f)return false;return true;}
#ifdef Q_OS_WIN
// Fixed application entropy: a DPAPI blob copied out of the vault file cannot be decrypted by
// another program that merely calls CryptUnprotectData with no entropy.
DATA_BLOB entropy(){
  static QByteArray salt=QByteArrayLiteral("REAzip/password-vault/v1");
  return DATA_BLOB{static_cast<DWORD>(salt.size()),reinterpret_cast<BYTE*>(salt.data())};
}
bool transform(const QByteArray& input,bool protect,QByteArray& output){
  DATA_BLOB in{static_cast<DWORD>(input.size()),reinterpret_cast<BYTE*>(const_cast<char*>(input.constData()))};
  auto salt=entropy();DATA_BLOB out{};
  const auto ok=protect
    ?CryptProtectData(&in,L"REAzip password vault",&salt,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)
    :CryptUnprotectData(&in,nullptr,&salt,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out);
  if(!ok)return false;
  output=QByteArray(reinterpret_cast<const char*>(out.pbData),static_cast<int>(out.cbData));
  SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);
  return true;
}
#endif
QJsonObject toJson(const QString& id,const QString& name,const QString& value,const QString& updated,qint64 stamp){
  return QJsonObject{{"id",id},{"name",name},{"secret",value},{"updated",updated},{"stamp",static_cast<double>(stamp)}};
}
}
Vault::Vault(QString directory,QObject* parent):QObject(parent),root(std::move(directory)),mode(defaultMode()){}
bool Vault::load(){
  items.clear();diagnostic.clear();readOnly=false;mode=defaultMode();
  if(root.isEmpty()||!QDir().mkpath(root)){report("Password vault directory is not writable: "+root);readOnly=true;emit changed();return false;}
  const auto path=root+"/vault.json";
  QFile file(path);
  if(!file.exists()){emit changed();return true;}
  if(!file.open(QIODevice::ReadOnly)||file.size()>kMaxFileBytes){report("Password vault cannot be read");readOnly=true;emit changed();return false;}
  QJsonParseError error{};const auto document=QJsonDocument::fromJson(file.readAll(),&error);file.close();
  if(error.error!=QJsonParseError::NoError||!document.isObject()){
    const auto backup=path+".corrupt-"+QString::number(QDateTime::currentMSecsSinceEpoch());
    if(QFile::rename(path,backup))report("Damaged password vault preserved at "+backup);
    else {report("Password vault is damaged and cannot be preserved; writing is disabled");readOnly=true;}
    emit changed();return false;
  }
  const auto object=document.object();
  if(object["schemaVersion"].toInt(1)>1){mode=object["protection"].toString(defaultMode());report("Password vault is from a newer version; writing is disabled");readOnly=true;emit changed();return false;}
  const auto protection=object["protection"].toString();
  QJsonArray entries;
  if(protection=="dpapi"){
#ifdef Q_OS_WIN
    QByteArray plain;
    QByteArray payload=QByteArray::fromBase64(object["payload"].toString().toLatin1());
    if(!transform(payload,false,plain)){report("Password vault cannot be decrypted for this Windows account");readOnly=true;emit changed();return false;}
    const auto inner=QJsonDocument::fromJson(plain);plain.fill(0);
    if(!inner.isObject()){report("Password vault content is not valid");readOnly=true;emit changed();return false;}
    entries=inner.object()["entries"].toArray();
#else
    mode=protection;readOnly=true;
    report("Password vault was encrypted by a Windows account and cannot be read here; it is left untouched");
    emit changed();return false;
#endif
  }else if(protection=="permissions"){
    entries=object["entries"].toArray();
#ifdef Q_OS_WIN
    // Windows can read the portable form; the next save upgrades it to DPAPI.
    mode="dpapi";
#endif
  }else{
    report("Password vault has an unknown protection scheme; writing is disabled");readOnly=true;emit changed();return false;
  }
  for(const auto& value:entries){
    const auto entry=value.toObject();
    const auto id=entry["id"].toString(),name=entry["name"].toString(),secret=entry["secret"].toString();
    if(!text(id,64)||!text(name,64)||!text(secret,1024))continue;
    if(items.size()>=kMaxEntries)break;
    items.append(Entry{id,name,secret,entry["updated"].toString(),static_cast<qint64>(entry["stamp"].toDouble())});
  }
  emit changed();
  return true;
}
QVariantList Vault::entries()const{
  auto sorted=items;std::sort(sorted.begin(),sorted.end(),[](const Entry& a,const Entry& b){return a.stamp>b.stamp;});
  QVariantList result;
  for(const auto& entry:sorted)result.append(QVariantMap{{"id",entry.id},{"name",entry.name},{"updated",entry.updated}});
  return result;
}
QString Vault::secret(const QString& id)const{for(const auto& entry:items)if(entry.id==id)return entry.value;return {};}
void Vault::report(const QString& message){diagnostic=message;}
void Vault::touch(const QString& id){
  for(auto& entry:items)if(entry.id==id){entry.stamp=QDateTime::currentMSecsSinceEpoch();entry.updated=now();save();emit changed();return;}
}
bool Vault::add(const QString& name,const QString& value){
  const auto label=name.trimmed();
  if(readOnly||!text(label,64)||!text(value,1024))return false;
  if(items.size()>=kMaxEntries)return false;
  items.append(Entry{QUuid::createUuid().toString(QUuid::WithoutBraces),label,value,now(),QDateTime::currentMSecsSinceEpoch()});
  if(!save()){items.removeLast();return false;}
  emit changed();return true;
}
bool Vault::remove(const QString& id){
  if(readOnly)return false;
  for(int i=0;i<items.size();++i)if(items[i].id==id){items.removeAt(i);if(!save()){load();return false;}emit changed();return true;}
  return false;
}
bool Vault::save(){
  if(readOnly){report("Password vault is read-only; change refused");return false;}
  QJsonArray entries;
  auto sorted=items;std::sort(sorted.begin(),sorted.end(),[](const Entry& a,const Entry& b){return a.stamp>b.stamp;});
  for(const auto& entry:sorted)entries.append(toJson(entry.id,entry.name,entry.value,entry.updated,entry.stamp));
  QJsonObject object{{"schemaVersion",1}};
#ifdef Q_OS_WIN
  QByteArray plain=QJsonDocument(QJsonObject{{"entries",entries}}).toJson(QJsonDocument::Compact);
  QByteArray payload;
  const auto encrypted=transform(plain,true,payload);plain.fill(0);
  if(!encrypted){report("Password vault encryption failed; nothing was written");return false;}
  object["protection"]="dpapi";object["payload"]=QString::fromLatin1(payload.toBase64());
#else
  object["protection"]="permissions";object["entries"]=entries;
#endif
  QSaveFile file(root+"/vault.json");
  if(!file.open(QIODevice::WriteOnly)){report("Password vault cannot be written: "+file.errorString());return false;}
  file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
  const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
  if(file.write(bytes)!=bytes.size()||!file.commit()){report("Password vault cannot be written: "+file.errorString());return false;}
  mode=object["protection"].toString();
  return true;
}
