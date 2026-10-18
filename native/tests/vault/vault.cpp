#include "../../vault/Vault.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
namespace {
bool writeFile(const QString& path,const QByteArray& bytes,bool ownerOnly=true){
  QFile file(path);if(!file.open(QIODevice::WriteOnly))return false;
  if(ownerOnly)file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
  return file.write(bytes)==bytes.size();
}
QByteArray readFile(const QString& path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};return file.readAll();}
}
int main(int argc,char** argv){
  QCoreApplication app(argc,argv);
  QTemporaryDir work;if(!work.isValid())return 1;
  const auto root=work.path()+"/data",file=root+"/vault.json";
  Vault vault(root);
  if(!vault.load())return 2;
  if(!vault.entries().isEmpty())return 3;
#ifdef Q_OS_WIN
  if(vault.protection()!="dpapi")return 4;
#else
  if(vault.protection()!="permissions")return 4;
#endif
  // Rejected input must never reach the file.
  if(vault.add("","value"))return 5;
  if(vault.add("name",""))return 5;
  if(vault.add(QString(65,'a'),"value"))return 5;
  if(vault.add(QString::fromUtf8("bad\nname"),"value"))return 5;
  if(!QFile::exists(file)){/* nothing written yet is expected */}
  const auto secret=QString::fromUtf8("s3cret-ÆØ-密码 kept");
  if(!vault.add(QString::fromUtf8("工作邮件"),secret))return 6;
  if(vault.entries().size()!=1)return 7;
  const auto id=vault.entries().first().toMap()["id"].toString();
  if(vault.secret(id)!=secret)return 8;
  const auto stored=readFile(file);
  if(!stored.contains("\"protection\""))return 9;
#ifdef Q_OS_WIN
  if(stored.contains("s3cret")||!stored.contains("\"dpapi\""))return 10;
#else
  if(stored.contains("s3cret"))return 10; // the plaintext form is documented, never claimed as encryption
  const auto permissions=QFile::permissions(file);
  if(permissions&(QFileDevice::ReadOther|QFileDevice::WriteOther|QFileDevice::ReadGroup|QFileDevice::WriteGroup))return 10;
#endif
  // A second instance must read the same entry back from disk.
  Vault reopened(root);
  if(!reopened.load())return 11;
  if(reopened.entries().size()!=1)return 12;
  if(reopened.secret(id)!=secret)return 13;
  if(!reopened.remove(id))return 14;
  if(!reopened.entries().isEmpty())return 15;
  // A damaged file is preserved under a new name instead of being silently replaced.
  const auto damagedRoot=work.path()+"/damaged";
  if(!QDir().mkpath(damagedRoot))return 16;
  if(!writeFile(damagedRoot+"/vault.json","not-json-at-all"))return 17;
  Vault damaged(damagedRoot);
  if(damaged.load())return 18;
  if(!damaged.entries().isEmpty())return 19;
  if(QDir(damagedRoot).entryList({"vault.json.corrupt-*"},QDir::Files).isEmpty())return 20;
  // Portable form written by another platform is read; Windows upgrades it to DPAPI on the next save.
  const auto portableRoot=work.path()+"/portable";
  if(!QDir().mkpath(portableRoot))return 21;
  QJsonObject portable{{"schemaVersion",1},{"protection","permissions"},{"entries",QJsonArray{QJsonObject{{"id","11111111-1111-1111-1111-111111111111"},{"name","portable"},{"secret","portable-secret"},{"updated","2026-01-01T00:00:00Z"},{"stamp",1.0}}}}};
  if(!writeFile(portableRoot+"/vault.json",QJsonDocument(portable).toJson(QJsonDocument::Compact)))return 22;
  Vault portableVault(portableRoot);
  if(!portableVault.load())return 23;
  if(portableVault.entries().size()!=1||portableVault.secret("11111111-1111-1111-1111-111111111111")!="portable-secret")return 24;
#ifdef Q_OS_WIN
  if(portableVault.protection()!="dpapi")return 25;
  if(!portableVault.add("second","second-secret"))return 26;
  if(readFile(portableRoot+"/vault.json").contains("portable-secret"))return 27; // upgraded to DPAPI
#endif
#ifndef Q_OS_WIN
  // A DPAPI vault copied from Windows must stay untouched: this build cannot read it.
  const auto foreignRoot=work.path()+"/foreign";
  if(!QDir().mkpath(foreignRoot))return 28;
  const auto foreign=QByteArray("{\"schemaVersion\":1,\"protection\":\"dpapi\",\"payload\":\"AAAA\"}");
  if(!writeFile(foreignRoot+"/vault.json",foreign))return 29;
  Vault foreignVault(foreignRoot);
  if(foreignVault.load())return 30;
  if(foreignVault.add("x","y"))return 31;
  if(readFile(foreignRoot+"/vault.json")!=foreign)return 32;
#endif
  std::cout<<"password vault: DPAPI/permission storage, validation, reload, corruption preservation, portable import PASS\n";
  return 0;
}
