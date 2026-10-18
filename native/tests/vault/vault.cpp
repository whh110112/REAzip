#include "../../vault/Vault.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>

// Every assertion returns a distinct code so the CTest log names the failure point even when
// stdout is buffered away. The reason string is printed to stderr, which CTest captures.
#define FAIL(code,reason) do{std::cerr<<"vault-test: FAIL "<<code<<" ("<<reason<<")"<<std::endl;return code;}while(false)

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
  QTemporaryDir work;if(!work.isValid())FAIL(1,"temporary directory unavailable");
  const auto root=work.path()+"/data",file=root+"/vault.json";
  Vault vault(root);
  if(!vault.load())FAIL(2,"load() on a missing vault reported an error: "+vault.error().toStdString());
  if(!vault.entries().isEmpty())FAIL(3,"a fresh vault reported entries");
#ifdef Q_OS_WIN
  if(vault.protection()!="dpapi")FAIL(4,"Windows vault protection is not dpapi");
#else
  if(vault.protection()!="permissions")FAIL(4,"non-Windows vault protection is not permissions");
#endif
  // Rejected input must never reach the file.
  if(vault.add("","value"))FAIL(5,"an empty name was accepted");
  if(vault.add("name",""))FAIL(5,"an empty secret was accepted");
  if(vault.add(QString(65,'a'),"value"))FAIL(5,"a 65-character name was accepted");
  if(vault.add(QString::fromUtf8("bad\nname"),"value"))FAIL(5,"a control character in the name was accepted");
  if(!QFile::exists(file)){/* nothing written yet is expected */}
  const auto secret=QString::fromUtf8("s3cret-ÆØ-密码 kept");
  if(!vault.add(QString::fromUtf8("工作邮件"),secret))FAIL(6,"adding a valid entry failed: "+vault.error().toStdString());
  if(vault.entries().size()!=1)FAIL(7,"the stored entry count is not 1");
  const auto id=vault.entries().first().toMap()["id"].toString();
  if(vault.secret(id)!=secret)FAIL(8,"the secret read back does not match the secret written");
  const auto stored=readFile(file);
  if(!stored.contains("\"protection\""))FAIL(9,"the vault file has no protection field");
#ifdef Q_OS_WIN
  if(stored.contains("s3cret")||!stored.contains("\"dpapi\""))FAIL(10,"the Windows vault file is not DPAPI-protected");
#else
  // This build has no system keyring, so the vault is stored as a plain-text JSON file that is
  // protected by owner-only permissions instead of encryption. That is the documented
  // behaviour (known-limitations.md), so the test asserts exactly that: the scheme is declared
  // as "permissions" and the payload is readable plain text. Claiming encryption here would be
  // a false statement about the build.
  if(QJsonDocument::fromJson(stored).object()["protection"].toString()!="permissions")
    FAIL(10,"the portable vault does not declare the permissions scheme");
  if(!stored.contains("s3cret"))FAIL(10,"the portable vault does not store the entry as plain text");
  const auto permissions=QFile::permissions(file);
  if(permissions&(QFileDevice::ReadOther|QFileDevice::WriteOther|QFileDevice::ReadGroup|QFileDevice::WriteGroup))
    FAIL(33,"the vault file is readable or writable by group/other");
  if(!(permissions&QFileDevice::ReadOwner))FAIL(33,"the vault file is not readable by its owner");
#endif
  // A second instance must read the same entry back from disk.
  Vault reopened(root);
  if(!reopened.load())FAIL(11,"reloading an existing vault failed: "+reopened.error().toStdString());
  if(reopened.entries().size()!=1)FAIL(12,"the reloaded vault does not have exactly one entry");
  if(reopened.secret(id)!=secret)FAIL(13,"the reloaded secret does not match");
  if(!reopened.remove(id))FAIL(14,"removing the entry failed: "+reopened.error().toStdString());
  if(!reopened.entries().isEmpty())FAIL(15,"the entry is still listed after removal");
  // A damaged file is preserved under a new name instead of being silently replaced.
  const auto damagedRoot=work.path()+"/damaged";
  if(!QDir().mkpath(damagedRoot))FAIL(16,"cannot create the damaged-vault directory");
  if(!writeFile(damagedRoot+"/vault.json","not-json-at-all"))FAIL(17,"cannot write the damaged vault file");
  Vault damaged(damagedRoot);
  if(damaged.load())FAIL(18,"a damaged vault was reported as loaded");
  if(!damaged.entries().isEmpty())FAIL(19,"a damaged vault reported entries");
  if(QDir(damagedRoot).entryList({"vault.json.corrupt-*"},QDir::Files).isEmpty())
    FAIL(20,"the damaged vault was not preserved under a .corrupt-* name");
  // Portable form written by another platform is read; Windows upgrades it to DPAPI on the next save.
  const auto portableRoot=work.path()+"/portable";
  if(!QDir().mkpath(portableRoot))FAIL(21,"cannot create the portable-vault directory");
  QJsonObject portable{{"schemaVersion",1},{"protection","permissions"},{"entries",QJsonArray{QJsonObject{{"id","11111111-1111-1111-1111-111111111111"},{"name","portable"},{"secret","portable-secret"},{"updated","2026-01-01T00:00:00Z"},{"stamp",1.0}}}}};
  if(!writeFile(portableRoot+"/vault.json",QJsonDocument(portable).toJson(QJsonDocument::Compact)))FAIL(22,"cannot write the portable vault file");
  Vault portableVault(portableRoot);
  if(!portableVault.load())FAIL(23,"the portable vault form was not readable: "+portableVault.error().toStdString());
  if(portableVault.entries().size()!=1||portableVault.secret("11111111-1111-1111-1111-111111111111")!="portable-secret")
    FAIL(24,"the portable vault entry was not imported correctly");
#ifdef Q_OS_WIN
  if(portableVault.protection()!="dpapi")FAIL(25,"the portable vault was not upgraded to dpapi on Windows");
  if(!portableVault.add("second","second-secret"))FAIL(26,"adding an entry to the upgraded vault failed");
  if(readFile(portableRoot+"/vault.json").contains("portable-secret"))FAIL(27,"the upgraded vault file still holds plain text");
#endif
#ifndef Q_OS_WIN
  // A DPAPI vault copied from Windows must stay untouched: this build cannot read it.
  const auto foreignRoot=work.path()+"/foreign";
  if(!QDir().mkpath(foreignRoot))FAIL(28,"cannot create the foreign-vault directory");
  const auto foreign=QByteArray("{\"schemaVersion\":1,\"protection\":\"dpapi\",\"payload\":\"AAAA\"}");
  if(!writeFile(foreignRoot+"/vault.json",foreign))FAIL(29,"cannot write the foreign vault file");
  Vault foreignVault(foreignRoot);
  if(foreignVault.load())FAIL(30,"a Windows DPAPI vault was reported as readable on this platform");
  if(foreignVault.add("x","y"))FAIL(31,"a write was accepted on a read-only foreign vault");
  if(readFile(foreignRoot+"/vault.json")!=foreign)FAIL(32,"the foreign vault file was modified");
#endif
  std::cout<<"password vault: platform storage, validation, reload, corruption preservation, portable import PASS"<<std::endl;
  return 0;
}
