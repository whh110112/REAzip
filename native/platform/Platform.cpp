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
#include <QProcess>
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
QString dataDirectory(QString directory){if(directory.isEmpty())directory=QCoreApplication::applicationDirPath();if(QFileInfo::exists(directory+"/portable.flag"))return directory+"/data";const auto current=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/native";
  if(QFileInfo::exists(current+"/settings.json")||QFileInfo::exists(current+"/vault/vault.json"))return current;
#ifdef Q_OS_WIN
  const auto base=qEnvironmentVariable("APPDATA");
#elif defined(Q_OS_MACOS)
  const auto base=QDir::homePath()+"/Library/Application Support";
#else
  const auto base=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#endif
  // Retain the existing profile in place; never copy secrets or overwrite a newer profile.
  for(const auto& old:{QString("REAzip/REAzip/native"),QString("REAzip/native"),QString("reazip/native")}){
    const auto path=base+'/'+old;if(QFileInfo::exists(path+"/settings.json")||QFileInfo::exists(path+"/vault/vault.json"))return path;
  }
  return current;}
Instance::Instance(QString directory,QObject* parent):QObject(parent),server(this){
  storage=directory;const bool existed=QFileInfo::exists(directory);if(!QDir().mkpath(directory))diagnostic="Cannot write data directory: "+directory+". Move the portable package to a writable folder.";else if(!existed)QFile::setPermissions(directory,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
  const auto hash=QCryptographicHash::hash(directory.toUtf8(),QCryptographicHash::Sha256).toHex().left(24);
#ifdef Q_OS_WIN
  name="ZENzip-"+QString::fromLatin1(hash);
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
int Instance::start(QJsonObject request){if(!diagnostic.isEmpty())return -1;if(!valid(request)){diagnostic="Invalid launch request";return -1;}
  if(lock->tryLock(0)){QLocalServer::removeServer(name);if(server.listen(name))return 0;diagnostic=server.errorString();return -1;}
  if(lock->error()==QLockFile::PermissionError){diagnostic="Cannot write data directory: "+storage+". Move the portable package to a writable folder.";return -1;}
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
QString defaultValue(QString path){auto p=wide(path);HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return {};DWORD size=0;auto status=RegQueryValueExW(key,nullptr,nullptr,nullptr,nullptr,&size);if(status!=ERROR_SUCCESS||size==0){RegCloseKey(key);return {};}std::wstring value(size/sizeof(wchar_t)+1,0);status=RegQueryValueExW(key,nullptr,nullptr,nullptr,reinterpret_cast<BYTE*>(value.data()),&size);RegCloseKey(key);return status==ERROR_SUCCESS?QString::fromWCharArray(value.c_str()):QString();}
const QStringList extensions={".zip",".zipx",".7z",".rar",".tar",".gz",".tgz",".bz2",".tbz",".tbz2",".xz",".txz",".lz",".lzma",".zst",".zstd",".cab",".iso",".wim",".arj",".lzh",".001"};
}
#endif
QString registerApplication(QString executable,QString prefix){
  if(!QFileInfo(executable).isAbsolute()||executable.contains('"'))return "Invalid application executable";
#ifdef Q_OS_WIN
  const auto classes=prefix+"Software\\Classes\\",capabilities=prefix+"Software\\ZENzip\\Capabilities";const auto command='"'+QDir::toNativeSeparators(executable)+'"';bool ok=true;
  auto put=[&](QString path,QString name,QString value){ok=set(path,name,value)&&ok;};
  // Upgrade only the legacy installation in this same directory. Keep its ProgID as an
  // alias so a Windows UserChoice pointing at REAzip.Archive continues to open archives.
  const auto legacyExe=QFileInfo(executable).dir().filePath("reazip.exe");
  const auto legacyProgram=classes+"REAzip.Archive";
  if(get(legacyProgram,"OwnerExecutable")==legacyExe){
    put(legacyProgram,"","ZENzip Archive");put(legacyProgram,"OwnerExecutable",executable);
    put(legacyProgram+"\\DefaultIcon","",command+",0");
    put(legacyProgram+"\\shell\\open\\command","",command+" \"%1\"");
    for(const auto& ext:extensions)for(const auto& action:{QString("extract_here"),QString("extract_to")}){
      const auto key=classes+"SystemFileAssociations\\"+ext+"\\shell\\REAzip."+action;
      if(get(key,"OwnerExecutable")==legacyExe)removeTree(key);
    }
    for(const auto& type:{QString("*"),QString("Directory")}){
      const auto key=classes+type+"\\shell\\REAzip.Add";if(get(key,"OwnerExecutable")==legacyExe)removeTree(key);
    }
    const auto app=classes+"Applications\\reazip.exe";if(get(app,"OwnerExecutable")==legacyExe)removeTree(app);
    const auto caps=prefix+"Software\\REAzip\\Capabilities";
    if(get(caps,"OwnerExecutable")==legacyExe){
      removeTree(caps);auto path=wide(prefix+"Software\\RegisteredApplications");HKEY key{};
      if(RegOpenKeyExW(HKEY_CURRENT_USER,path.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS){RegDeleteValueW(key,L"REAzip");RegCloseKey(key);}
    }
  }

  put(classes+"ZENzip.Archive","","ZENzip Archive");put(classes+"ZENzip.Archive","OwnerExecutable",executable);put(classes+"ZENzip.Archive","FriendlyTypeName","ZENzip Archive");put(classes+"ZENzip.Archive\\DefaultIcon","",command+",0");put(classes+"ZENzip.Archive\\shell\\open\\command","",command+" \"%1\"");
  put(capabilities,"ApplicationName","ZENzip");put(capabilities,"ApplicationDescription","ZENzip native archive manager");put(capabilities,"ApplicationIcon",command+",0");put(capabilities,"OwnerExecutable",executable);
  put(prefix+"Software\\RegisteredApplications","ZENzip","Software\\ZENzip\\Capabilities");
  // Classic shell verbs. Windows 11 lists them under "Show more options": the current menu requires a
  // signed IExplorerCommand COM server with package identity, which this build does not ship.
  for(const auto& ext:extensions){put(capabilities+"\\FileAssociations",ext,"ZENzip.Archive");put(classes+ext+"\\OpenWithProgids","ZENzip.Archive","");
    for(const auto& action:{QString("extract_here"),QString("extract_to")}){const auto key=classes+"SystemFileAssociations\\"+ext+"\\shell\\ZENzip."+action;put(key,"",action=="extract_here"?"ZENzip: Extract here":"ZENzip: Extract to folder...");put(key,"Icon",command+",0");put(key,"OwnerExecutable",executable);
      // Multi-select is only offered where every selected file is handled independently.
      put(key,"MultiSelectModel",action=="extract_here"?"Player":"Single");put(key+"\\command","",command+" --"+QString(action).replace('_','-')+" \"%1\"");}
  }
  for(const auto& type:{QString("*"),QString("Directory")}){const auto key=classes+type+"\\shell\\ZENzip.Add";put(key,"","ZENzip: Add to archive...");put(key,"Icon",command+",0");put(key,"OwnerExecutable",executable);put(key,"MultiSelectModel",type=="*"?"Player":"Single");put(key+"\\command","",command+" --add \"%1\"");}
  // An entry in the "Open with" list for files whose extension is not tied to the ProgID.
  const auto application=classes+"Applications\\"+QFileInfo(executable).fileName();
  put(application,"FriendlyAppName","ZENzip");put(application,"OwnerExecutable",executable);put(application+"\\DefaultIcon","",command+",0");put(application+"\\shell\\open\\command","",command+" \"%1\"");
  for(const auto& ext:extensions)put(application+"\\SupportedTypes",ext,"");
  if(prefix.isEmpty())SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);return ok?QString():"Could not write per-user association registration";
#elif defined(Q_OS_MACOS)
  // Types are declared in Info.plist inside the bundle; macOS keeps its own default-handler database,
  // so this call only means "declared here". Finder decides the actual handler.
  Q_UNUSED(executable);Q_UNUSED(prefix);return {};
#else
  Q_UNUSED(prefix);const auto appImage=qEnvironmentVariable("APPIMAGE");if(!appImage.isEmpty())executable=QFileInfo(appImage).canonicalFilePath();if(executable.startsWith("/tmp/.mount_")||executable.isEmpty())return "Copy the AppImage to a permanent location before integration";
  auto quoted=executable;quoted.replace('\\',"\\\\").replace('"',"\\\"").replace('`',"\\`").replace('$',"\\$").replace('%',"%%");
  const auto folder=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications";if(!QDir().mkpath(folder))return "Cannot create applications directory";
  const QStringList types={"application/zip","application/x-7z-compressed","application/vnd.rar","application/x-tar","application/gzip","application/x-bzip2","application/x-xz","application/zstd","application/x-lzma","application/x-cd-image","application/x-iso9660-image","application/x-ms-wim","application/x-archive","application/x-arj","application/x-lzh-compressed"};
  QSaveFile file(folder+"/com.zenzip.app.desktop");if(!file.open(QIODevice::WriteOnly))return file.errorString();
  const auto data=QString("[Desktop Entry]\nType=Application\nName=ZENzip\nGenericName=Archive Manager\nComment=Open, create and extract archives\nExec=\"")+quoted+"\" %F\nTryExec="+executable+"\nIcon=package-x-generic\nTerminal=false\nCategories=Utility;Archiving;\nMimeType="+types.join(';')+";\nActions=ExtractHere;ExtractTo;\nX-ZENzip-Owner="+executable+"\n\n[Desktop Action ExtractHere]\nName=Extract here\nExec=\""+quoted+"\" --extract-here %f\n\n[Desktop Action ExtractTo]\nName=Extract to folder...\nExec=\""+quoted+"\" --extract-to %f\n";
  file.write(data.toUtf8());if(!file.commit())return file.errorString();
  // update-desktop-database refreshes the MIME cache and xdg-mime writes mimeapps.list, so the handler
  // is selected rather than merely installed. A missing helper is not an error.
  const auto database=QStandardPaths::findExecutable("update-desktop-database");if(!database.isEmpty())QProcess::execute(database,{folder});
  const auto xdg=QStandardPaths::findExecutable("xdg-mime");if(!xdg.isEmpty())for(const auto& type:types)QProcess::execute(xdg,{"default","com.zenzip.app.desktop",type});
  return {};
#endif
}
QString unregisterApplication(QString executable,QString prefix){
#ifdef Q_OS_WIN
  const auto classes=prefix+"Software\\Classes\\",capabilities=prefix+"Software\\ZENzip\\Capabilities";
  if(get(classes+"ZENzip.Archive","OwnerExecutable")!=executable)return {};
  if(get(classes+"REAzip.Archive","OwnerExecutable")==executable)removeTree(classes+"REAzip.Archive");
  removeTree(classes+"ZENzip.Archive");if(get(capabilities,"OwnerExecutable")==executable){removeTree(capabilities);if(get(prefix+"Software\\RegisteredApplications","ZENzip")=="Software\\ZENzip\\Capabilities"){auto p=wide(prefix+"Software\\RegisteredApplications");HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS){RegDeleteValueW(key,L"ZENzip");RegCloseKey(key);}}}
  const auto application=classes+"Applications\\"+QFileInfo(executable).fileName();if(get(application,"OwnerExecutable")==executable)removeTree(application);
  for(const auto& ext:extensions){auto p=wide(classes+ext+"\\OpenWithProgids");HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS){RegDeleteValueW(key,L"ZENzip.Archive");RegCloseKey(key);}for(const auto& action:{QString("extract_here"),QString("extract_to")}){const auto k=classes+"SystemFileAssociations\\"+ext+"\\shell\\ZENzip."+action;if(get(k,"OwnerExecutable")==executable)removeTree(k);}}
  for(const auto& type:{QString("*"),QString("Directory")}){auto k=classes+type+"\\shell\\ZENzip.Add";if(get(k,"OwnerExecutable")==executable)removeTree(k);}
  if(prefix.isEmpty())SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);return {};
#else
  Q_UNUSED(prefix);const auto path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications/com.zenzip.app.desktop";QFile file(path);if(file.open(QIODevice::ReadOnly)&&file.readAll().contains(("X-ZENzip-Owner="+executable+"\n").toUtf8())){file.close();if(!file.remove())return file.errorString();}return {};
#endif
}
bool propagateOrigin(QString archive,QString destination,QString* error){
#ifdef Q_OS_WIN
  QFile source(archive+":Zone.Identifier");if(!source.exists())return true;if(!source.open(QIODevice::ReadOnly)){if(error)*error="Cannot read archive origin metadata";return false;}const auto zone=source.read(65537);if(zone.size()>65536){if(error)*error="Archive origin metadata exceeds limit";return false;}
  auto target=QDir::toNativeSeparators(destination)+":Zone.Identifier";if(!target.startsWith("\\\\?\\"))target=target.startsWith("\\\\")?"\\\\?\\UNC\\"+target.mid(2):"\\\\?\\"+target;
  auto handle=CreateFileW(reinterpret_cast<const wchar_t*>(target.utf16()),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(handle==INVALID_HANDLE_VALUE){if(error)*error="Cannot preserve archive origin metadata";return false;}DWORD written{};const bool ok=WriteFile(handle,zone.constData(),static_cast<DWORD>(zone.size()),&written,nullptr)&&written==static_cast<DWORD>(zone.size())&&FlushFileBuffers(handle);CloseHandle(handle);if(!ok&&error)*error="Cannot write archive origin metadata";return ok;
#else
  Q_UNUSED(archive);Q_UNUSED(destination);Q_UNUSED(error);return true;
#endif
}
QString associationSummary(QString executable,QString prefix){
#ifdef Q_OS_WIN
  const auto classes=prefix+"Software\\Classes\\";QStringList lines;
  const auto program=defaultValue(classes+"ZENzip.Archive");
  lines<<QString("Archive ProgID: %1").arg(program.isEmpty()?QString("not registered"):program);
  int verbs=0,icons=0;
  for(const auto& ext:extensions){const auto key=classes+"SystemFileAssociations\\"+ext+"\\shell\\ZENzip.extract_here";if(!get(key,"").isEmpty()){++verbs;if(!get(key,"Icon").isEmpty())++icons;}}
  lines<<QString("Context menu: %1 of %2 extensions (%3 with icon)").arg(verbs).arg(extensions.size()).arg(icons);
  lines<<QString("Add to archive: %1").arg(get(classes+"*\\shell\\ZENzip.Add","").isEmpty()?"missing":"files and folders");
  lines<<QString("Open with list: %1").arg(get(classes+"Applications\\"+QFileInfo(executable).fileName(),"FriendlyAppName").isEmpty()?"missing":"registered");
  lines<<QString("Default programs entry: %1").arg(get(prefix+"Software\\RegisteredApplications","ZENzip").isEmpty()?"missing":"registered");
  lines<<"Windows 11 modern menu: not available (needs a signed COM server)";
  return lines.join('\n');
#elif defined(Q_OS_MACOS)
  Q_UNUSED(executable);Q_UNUSED(prefix);
  return "Declared in Info.plist: zip, 7z, rar, tar, gz, bz2, xz, zst, iso, wim\nDefault handler: Finder -> Get Info -> Open With -> Change All";
#else
  Q_UNUSED(executable);Q_UNUSED(prefix);QStringList lines;
  const auto path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications/com.zenzip.app.desktop";
  lines<<QString("Desktop entry: %1").arg(QFileInfo::exists(path)?"installed":"missing");
  const auto xdg=QStandardPaths::findExecutable("xdg-mime");
  lines<<QString("xdg-mime: %1").arg(xdg.isEmpty()?"not installed":"available");
  if(!xdg.isEmpty()){QProcess process;process.start(xdg,{"query","default","application/zip"});if(process.waitForFinished(3000)){const auto handler=QString::fromUtf8(process.readAllStandardOutput()).trimmed();lines<<QString("Default for application/zip: %1").arg(handler.isEmpty()?QString("not set"):handler);}}
  return lines.join('\n');
#endif
}
}
