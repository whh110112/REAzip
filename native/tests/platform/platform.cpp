#include "../../platform/Platform.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QJsonArray>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);app.setOrganizationName("REAzip");app.setApplicationName("PlatformTests");
  auto request=QJsonObject{{"version",1},{"action","open"},{"paths",QJsonArray{QDir::tempPath()+QString::fromUtf8("/中文 space.zip")}}};
  if(app.arguments().size()==3&&app.arguments()[1]=="--send"){platform::Instance instance(app.arguments()[2]);return instance.start(request)==1?0:2;}
  QTemporaryDir work;if(!work.isValid())return 3;
  QFile marker(work.path()+"/portable.flag");if(!marker.open(QIODevice::WriteOnly))return 4;marker.close();if(platform::dataDirectory(work.path())!=work.path()+"/data")return 5;
  auto invalid=request;invalid["paths"]=QJsonArray{"relative.zip"};if(platform::Instance::valid(invalid))return 6;
  const auto exe=QCoreApplication::applicationFilePath();
#ifdef Q_OS_WIN
  const auto scope="HKEY_CURRENT_USER\\Software\\REAzipTests\\"+QFileInfo(work.path()).fileName();QSettings registry(scope,QSettings::NativeFormat);const auto prefix=scope.mid(QString("HKEY_CURRENT_USER\\").size())+"\\";
  if(!platform::registerApplication(exe,prefix).isEmpty())return 7;
  registry.sync();if(registry.value("Software/Classes/REAzip.Archive/shell/open/command/.").toString()!='"'+QDir::toNativeSeparators(exe)+"\" \"%1\"")return 8;
  // Shell verbs must carry an icon and declare multi-select support, otherwise Explorer shows a
  // nameless entry and disables the command for selections.
  if(registry.value("Software/Classes/SystemFileAssociations/.zip/shell/REAzip.extract_here/Icon").toString().isEmpty())return 40;
  if(registry.value("Software/Classes/SystemFileAssociations/.zip/shell/REAzip.extract_here/MultiSelectModel").toString()!="Player")return 41;
  if(registry.value("Software/Classes/SystemFileAssociations/.7z/shell/REAzip.extract_to/MultiSelectModel").toString()!="Single")return 42;
  if(registry.value("Software/Classes/*/shell/REAzip.Add/MultiSelectModel").toString()!="Player")return 43;
  if(registry.value("Software/Classes/Directory/shell/REAzip.Add/Icon").toString().isEmpty())return 44;
  if(!registry.contains("Software/Classes/Applications/"+QFileInfo(exe).fileName()+"/SupportedTypes/.rar"))return 45;
  // Windows 11 can only surface these verbs in the classic menu; the summary has to say so.
  if(!platform::associationSummary(exe,prefix).contains("modern menu"))return 46;
  registry.setValue("Software/Classes/REAzip.Archive/OwnerExecutable","other-app");registry.setValue("Software/Classes/.zip/Unrelated","keep");registry.sync();platform::unregisterApplication(exe,prefix);registry.sync();if(registry.value("Software/Classes/REAzip.Archive/OwnerExecutable")!="other-app")return 9;
  registry.setValue("Software/Classes/REAzip.Archive/OwnerExecutable",exe);registry.sync();platform::unregisterApplication(exe,prefix);registry.sync();if(registry.contains("Software/Classes/REAzip.Archive/OwnerExecutable")||registry.value("Software/Classes/.zip/Unrelated")!="keep")return 10;registry.clear();registry.sync();
  const auto archive=work.path()+"/download.zip",output=work.path()+"/output.txt";for(const auto& p:{archive,output}){QFile f(p);if(!f.open(QIODevice::WriteOnly))return 11;f.write("bytes");}
  QFile zone(archive+":Zone.Identifier");if(!zone.open(QIODevice::WriteOnly))return 12;const QByteArray bytes="[ZoneTransfer]\r\nZoneId=3\r\n";zone.write(bytes);zone.close();QString error;if(!platform::propagateOrigin(archive,output,&error))return 13;QFile copied(output+":Zone.Identifier");if(!copied.open(QIODevice::ReadOnly)||copied.readAll()!=bytes)return 14;
#elif !defined(Q_OS_MACOS)
  qputenv("XDG_DATA_HOME",work.path().toUtf8());if(!platform::registerApplication(exe).isEmpty())return 7;QFile desktop(work.path()+"/applications/com.reazip.app.desktop");if(!desktop.open(QIODevice::ReadOnly))return 8;const auto entry=QString::fromUtf8(desktop.readAll());desktop.close();
  if(!entry.contains(" %F")||!entry.contains("MimeType=application/zip;")||!entry.contains("Actions=ExtractHere;ExtractTo;")||!entry.contains("--extract-here %f"))return 8;
  if(!platform::associationSummary(exe).contains("Desktop entry: installed"))return 40;
  platform::unregisterApplication(exe);if(desktop.exists())return 9;
#endif
  platform::Instance primary(work.path()+"/instance");if(primary.start(request)!=0)return 15;
  bool received=false;QObject::connect(&primary,&platform::Instance::request,&app,[&](QJsonObject data){received=data==request;});
  QProcess child;QObject::connect(&child,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),&app,[&](int code,QProcess::ExitStatus status){if(code==0&&status==QProcess::NormalExit&&received){std::cout<<"same-user launch forwarding, path validation, portable paths, owned registration and origin metadata PASS\n";app.exit(0);}else{std::cerr<<"forward failed "<<code<<" "<<received<<'\n';app.exit(16);}});
  child.start(exe,{"--send",work.path()+"/instance"});QTimer::singleShot(10000,&app,[&]{child.kill();app.exit(17);});return app.exec();
}
