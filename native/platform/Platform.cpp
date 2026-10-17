#include "Platform.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLocalSocket>
#include <QTimer>
#include <QFileOpenEvent>
#include <QFile>
#include <QSaveFile>
#include <QDirIterator>
#include <QUrl>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#ifdef Q_OS_MACOS
#include <mach/mach.h>
#endif
#endif
namespace platform {
quint64 residentBytes(){
#ifdef Q_OS_WIN
  PROCESS_MEMORY_COUNTERS counters{};return GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))?counters.WorkingSetSize:0;
#elif defined(Q_OS_MACOS)
  mach_task_basic_info_data_t info{};mach_msg_type_number_t count=MACH_TASK_BASIC_INFO_COUNT;return task_info(mach_task_self(),MACH_TASK_BASIC_INFO,reinterpret_cast<task_info_t>(&info),&count)==KERN_SUCCESS?info.resident_size:0;
#else
  QFile file("/proc/self/statm");if(!file.open(QIODevice::ReadOnly))return 0;const auto parts=file.readAll().simplified().split(' ');return parts.size()>1?parts[1].toULongLong()*static_cast<quint64>(sysconf(_SC_PAGESIZE)):0;
#endif
}
QString dataDirectory(QString directory){if(directory.isEmpty())directory=QCoreApplication::applicationDirPath();if(QFileInfo::exists(directory+"/portable.flag"))return directory+"/data";return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/native";}
Instance::Instance(QString directory,QObject* parent):QObject(parent),server(this){
  QDir().mkpath(directory);QFile::setPermissions(directory,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
  const auto hash=QCryptographicHash::hash(directory.toUtf8(),QCryptographicHash::Sha256).toHex().left(24);
#ifdef Q_OS_WIN
  name="REAzip-"+QString::fromLatin1(hash);
#else
  name=directory+"/instance.sock";
#endif
  lock=std::make_unique<QLockFile>(directory+"/instance.lock");server.setSocketOptions(QLocalServer::UserAccessOption);server.setMaxPendingConnections(8);
  connect(&server,&QLocalServer::newConnection,this,[this]{while(auto socket=server.nextPendingConnection()){
#ifndef Q_OS_WIN
    bool allowed=false;
#ifdef Q_OS_MACOS
    uid_t uid;gid_t gid;allowed=getpeereid(static_cast<int>(socket->socketDescriptor()),&uid,&gid)==0&&uid==getuid();
#else
    struct ucred credentials{};socklen_t size=sizeof(credentials);allowed=getsockopt(static_cast<int>(socket->socketDescriptor()),SOL_SOCKET,SO_PEERCRED,&credentials,&size)==0&&credentials.uid==getuid();
#endif
    if(!allowed){socket->abort();socket->deleteLater();continue;}
#endif
    socket->setReadBufferSize(1024*1024+1);connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
    QTimer::singleShot(3000,socket,[socket]{socket->abort();});
    auto bytes=std::make_shared<QByteArray>();connect(socket,&QLocalSocket::readyRead,this,[this,socket,bytes]{
      *bytes+=socket->readAll();if(bytes->size()>1024*1024){socket->abort();return;}auto end=bytes->indexOf('\n');if(end<0)return;
      const auto doc=QJsonDocument::fromJson(bytes->left(end));if(!doc.isObject()||!valid(doc.object())){socket->abort();return;}
      socket->write("OK\n");socket->flush();socket->disconnectFromServer();emit request(doc.object());
    });
  }});
}
bool Instance::valid(const QJsonObject& request){if(request["version"].toInt()!=1)return false;const auto action=request["action"].toString();if(action!="open"&&action!="extract_here"&&action!="extract_to"&&action!="add")return false;
  if(!request["paths"].isArray()||request["paths"].toArray().size()>128)return false;for(const auto& v:request["paths"].toArray())if(!v.isString()||!QFileInfo(v.toString()).isAbsolute()||v.toString().contains(QChar::Null))return false;return true;}
int Instance::start(QJsonObject request){if(!valid(request)){diagnostic="Invalid launch request";return -1;}
  if(lock->tryLock(0)){QLocalServer::removeServer(name);if(server.listen(name))return 0;diagnostic=server.errorString();return -1;}
  for(int attempt=0;attempt<5;++attempt){QLocalSocket socket;socket.connectToServer(name);if(!socket.waitForConnected(300))continue;
    const auto bytes=QJsonDocument(request).toJson(QJsonDocument::Compact)+'\n';socket.write(bytes);if(!socket.waitForBytesWritten(500))continue;if(socket.waitForReadyRead(1000)&&socket.readLine()=="OK\n")return 1;}
  diagnostic="Existing instance did not accept the request; retry after it exits";return -1;
}
bool OpenEvents::eventFilter(QObject*,QEvent* event){if(event->type()==QEvent::FileOpen){auto openEvent=static_cast<QFileOpenEvent*>(event);emit open(openEvent->file());return true;}return false;}
#ifdef Q_OS_WIN
namespace {
std::wstring wide(const QString& v){return v.toStdWString();}
bool set(QString path,QString name,QString value){HKEY key{};auto p=wide(path),n=wide(name),v=wide(value);if(RegCreateKeyExW(HKEY_CURRENT_USER,p.c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return false;auto status=RegSetValueExW(key,n.c_str(),0,REG_SZ,reinterpret_cast<const BYTE*>(v.c_str()),static_cast<DWORD>((v.size()+1)*sizeof(wchar_t)));RegCloseKey(key);return status==ERROR_SUCCESS;}
QString get(QString path,QString name){auto p=wide(path),n=wide(name);DWORD size=0;if(RegGetValueW(HKEY_CURRENT_USER,p.c_str(),n.c_str(),RRF_RT_REG_SZ,nullptr,nullptr,&size)!=ERROR_SUCCESS)return {};std::wstring value(size/sizeof(wchar_t),0);if(RegGetValueW(HKEY_CURRENT_USER,p.c_str(),n.c_str(),RRF_RT_REG_SZ,nullptr,value.data(),&size)!=ERROR_SUCCESS)return {};return QString::fromWCharArray(value.c_str());}
void removeTree(QString path){auto p=wide(path);RegDeleteTreeW(HKEY_CURRENT_USER,p.c_str());}
const QStringList extensions={".zip",".7z",".rar",".tar",".gz",".tgz",".bz2",".xz",".iso",".wim",".001"};
}
#endif
QString registerApplication(QString executable,QString prefix){
  if(!QFileInfo(executable).isAbsolute()||executable.contains('"'))return "Invalid application executable";
#ifdef Q_OS_WIN
  const auto classes=prefix+"Software\\Classes\\",capabilities=prefix+"Software\\REAzip\\Capabilities";const auto command='"'+QDir::toNativeSeparators(executable)+'"';bool ok=true;
  auto put=[&](QString path,QString name,QString value){ok=set(path,name,value)&&ok;};
  put(classes+"REAzip.Archive","","REAzip Archive");put(classes+"REAzip.Archive","OwnerExecutable",executable);put(classes+"REAzip.Archive\\DefaultIcon","",command+",0");put(classes+"REAzip.Archive\\shell\\open\\command","",command+" \"%1\"");
  put(capabilities,"ApplicationName","REAzip");put(capabilities,"ApplicationDescription","REAzip native archive manager");put(capabilities,"OwnerExecutable",executable);
  put(prefix+"Software\\RegisteredApplications","REAzip","Software\\REAzip\\Capabilities");
  for(const auto& ext:extensions){put(capabilities+"\\FileAssociations",ext,"REAzip.Archive");put(classes+ext+"\\OpenWithProgids","REAzip.Archive","");
    for(const auto& action:{QString("extract_here"),QString("extract_to")}){const auto key=classes+"SystemFileAssociations\\"+ext+"\\shell\\REAzip."+action;put(key,"",action=="extract_here"?"REAzip: Extract here":"REAzip: Extract to folder...");put(key,"OwnerExecutable",executable);put(key+"\\command","",command+" --"+QString(action).replace('_','-')+" \"%1\"");}
  }
  for(const auto& type:{QString("*"),QString("Directory")}){const auto key=classes+type+"\\shell\\REAzip.Add";put(key,"","REAzip: Add to archive...");put(key,"OwnerExecutable",executable);put(key+"\\command","",command+" --add \"%1\"");}
  if(prefix.isEmpty())SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);return ok?QString():"Could not write per-user association registration";
#elif defined(Q_OS_MACOS)
  Q_UNUSED(prefix);return "File types are declared in the installed application bundle; choose REAzip in Finder Get Info / Open With";
#else
  Q_UNUSED(prefix);const auto appImage=qEnvironmentVariable("APPIMAGE");if(!appImage.isEmpty())executable=QFileInfo(appImage).canonicalFilePath();if(executable.startsWith("/tmp/.mount_")||executable.isEmpty())return "Copy AppImage to a permanent location before integration";
  auto quoted=executable;quoted.replace('\\',"\\\\").replace('"',"\\\"").replace('`',"\\`").replace('$',"\\$").replace('%',"%%");
  const auto folder=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications";if(!QDir().mkpath(folder))return "Cannot create applications directory";
  QSaveFile file(folder+"/com.reazip.app.desktop");if(!file.open(QIODevice::WriteOnly))return file.errorString();const auto data="[Desktop Entry]\nType=Application\nName=REAzip\nExec=\""+quoted+"\" %F\nIcon=package-x-generic\nTerminal=false\nCategories=Utility;Archiving;\nMimeType=application/zip;application/x-7z-compressed;application/vnd.rar;application/x-tar;application/gzip;application/x-cd-image;\nX-REAzip-Owner="+executable+"\n";file.write(data.toUtf8());return file.commit()?QString():file.errorString();
#endif
}
QString unregisterApplication(QString executable,QString prefix){
#ifdef Q_OS_WIN
  const auto classes=prefix+"Software\\Classes\\",capabilities=prefix+"Software\\REAzip\\Capabilities";
  if(get(classes+"REAzip.Archive","OwnerExecutable")!=executable)return {};
  removeTree(classes+"REAzip.Archive");if(get(capabilities,"OwnerExecutable")==executable){removeTree(capabilities);if(get(prefix+"Software\\RegisteredApplications","REAzip")=="Software\\REAzip\\Capabilities"){auto p=wide(prefix+"Software\\RegisteredApplications");HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS){RegDeleteValueW(key,L"REAzip");RegCloseKey(key);}}}
  for(const auto& ext:extensions){auto p=wide(classes+ext+"\\OpenWithProgids");HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS){RegDeleteValueW(key,L"REAzip.Archive");RegCloseKey(key);}for(const auto& action:{QString("extract_here"),QString("extract_to")}){const auto k=classes+"SystemFileAssociations\\"+ext+"\\shell\\REAzip."+action;if(get(k,"OwnerExecutable")==executable)removeTree(k);}}
  for(const auto& type:{QString("*"),QString("Directory")}){auto k=classes+type+"\\shell\\REAzip.Add";if(get(k,"OwnerExecutable")==executable)removeTree(k);}
  if(prefix.isEmpty())SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);return {};
#else
  Q_UNUSED(prefix);const auto path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications/com.reazip.app.desktop";QFile file(path);if(file.open(QIODevice::ReadOnly)&&file.readAll().contains(("X-REAzip-Owner="+executable+"\n").toUtf8())){file.close();if(!file.remove())return file.errorString();}return {};
#endif
}
bool propagateOrigin(QString archive,QString destination,QString* error){
#ifdef Q_OS_WIN
  QFile source(archive+":Zone.Identifier");if(!source.exists())return true;if(!source.open(QIODevice::ReadOnly)){if(error)*error="Cannot read archive origin metadata";return false;}const auto zone=source.read(65537);if(zone.size()>65536){if(error)*error="Archive origin metadata exceeds limit";return false;}
  QFile target(destination+":Zone.Identifier");if(!target.open(QIODevice::WriteOnly|QIODevice::Truncate)||target.write(zone)!=zone.size()){if(error)*error="Cannot preserve archive origin metadata";return false;}return target.flush();
#else
  Q_UNUSED(archive);Q_UNUSED(destination);Q_UNUSED(error);return true;
#endif
}
}
