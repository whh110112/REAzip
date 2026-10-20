#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QImage>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QFileInfo>
#include "AppController.hpp"
#include "../platform/Platform.hpp"
#include <QJsonArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QSaveFile>
#include <QPainter>
#include <QIcon>
#include <QFontDatabase>
int main(int argc, char** argv) {
  QElapsedTimer startup;startup.start();qint64 firstFrame=-1,archiveReady=-1;
  QGuiApplication app(argc, argv);
  QGuiApplication::setOrganizationName("ZENzip");
  QGuiApplication::setApplicationName("ZENzip");
  QCoreApplication::setApplicationVersion(ZENZIP_VERSION_FULL);
  // OS/taskbar icons keep the fixed A mark; archive metrics and skins only
  // affect the in-window component, never registered file associations.
  QIcon applicationIcon;
  for (const auto size : {16,20,24,32,48,64,128,256}) {
    QPixmap mark(size,size);mark.fill(Qt::transparent);
    QPainter painter(&mark);painter.setRenderHint(QPainter::Antialiasing);painter.scale(size/256.0,size/256.0);
    painter.setBrush(QColor("#FAF9F6"));painter.setPen(Qt::NoPen);painter.drawRoundedRect(QRectF(0,0,256,256),56,56);
    painter.setBrush(Qt::NoBrush);painter.setPen(QPen(QColor("#486451"),size<=24?32:24,Qt::SolidLine,Qt::RoundCap));
    painter.drawArc(QRectF(44,44,168,168),21*16,-312*16);painter.end();applicationIcon.addPixmap(mark);
  }
  app.setWindowIcon(applicationIcon);
  QQuickStyle::setStyle("Basic");
  const auto arguments=app.arguments();
  const auto fontIndex=arguments.indexOf("--font-file");
  if(arguments.contains("--smoke")&&fontIndex>=0&&fontIndex+1<arguments.size()){
    const auto id=QFontDatabase::addApplicationFont(arguments[fontIndex+1]);
    const auto families=QFontDatabase::applicationFontFamilies(id);
    if(families.isEmpty()){qCritical()<<"Cannot load capture font";return 7;}
    app.setFont(QFont(families.first()));
  }
  if(arguments.contains("--register"))return platform::registerApplication(app.applicationFilePath()).isEmpty()?0:2;
  if(arguments.contains("--unregister"))return platform::unregisterApplication(app.applicationFilePath()).isEmpty()?0:2;
  QJsonArray launchPaths;QString action="open";
  for(int i=1;i<arguments.size();++i){const auto arg=arguments[i];if(arg=="--font-file"||arg=="--screenshot"||arg=="--design-scene"||arg=="--theme"||arg=="--language"||arg=="--performance"){++i;continue;}if(arg=="--extract-here")action="extract_here";else if(arg=="--extract-to")action="extract_to";else if(arg=="--add")action="add";else if(!arg.startsWith('-')&&QFileInfo::exists(arg))launchPaths.append(QFileInfo(arg).absoluteFilePath());}
  QJsonObject launch{{"version",1},{"action",action},{"paths",launchPaths}};
  QTemporaryDir smokeData;const bool testing=arguments.contains("--smoke")||arguments.contains("--design-preview");QString startupFailure;
  platform::Instance instance(testing?smokeData.path()+"/instance":platform::dataDirectory());
  if(!testing){const auto state=instance.start(launch);if(state==1)return 0;if(state<0){startupFailure=instance.error();qCritical().noquote()<<startupFailure;}}
#ifdef Q_OS_WIN
  const auto worker=QCoreApplication::applicationDirPath()+"/zenzip-worker.exe";
  const auto library=QCoreApplication::applicationDirPath()+"/engine/7z.dll";
#else
  const auto worker=QCoreApplication::applicationDirPath()+"/zenzip-worker";
  const auto library=QCoreApplication::applicationDirPath()+"/engine/7z.so";
#endif
  AppController controller(worker,library,nullptr,testing||!startupFailure.isEmpty()?smokeData.path():QString());
  // Explicit acceptance mode for testing HTTPS from an isolated package.
  // Ordinary startup and the regular offline smoke test never contact GitHub.
  auto updates=qobject_cast<UpdateChecker*>(controller.updates());
  const bool checkUpdateSmoke=arguments.contains("--smoke")&&arguments.contains("--check-updates");
  if(checkUpdateSmoke)QTimer::singleShot(0,updates,&UpdateChecker::check);
  if(arguments.contains("--smoke"))QObject::connect(&controller,&AppController::message,&app,[&](QString text,bool error){if(error){qCritical().noquote()<<text;app.exit(6);}});
  for(const auto& option:{QString("--theme"),QString("--language")}){const auto index=arguments.indexOf(option);if(index>=0&&index+1<arguments.size())controller.setPreference(option=="--theme"?"skin":"language",arguments[index+1]);}
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("backend",app.arguments().contains("--design-preview")||!startupFailure.isEmpty()?nullptr:static_cast<QObject*>(&controller));
  engine.rootContext()->setContextProperty("designPreview",app.arguments().contains("--design-preview"));
  engine.rootContext()->setContextProperty("startupFailure",startupFailure);
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule("ZENzip", "Main");
  if(!engine.rootObjects().isEmpty())if(auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()))QObject::connect(window,&QQuickWindow::frameSwapped,&app,[&]{if(firstFrame<0)firstFrame=startup.elapsed();});
  QObject::connect(&controller,&AppController::operationCompleted,&app,[&](QString operation,QJsonObject){if(operation=="list"&&archiveReady<0)archiveReady=startup.elapsed();});
  auto dispatch=[&](QJsonObject request){controller.handleLaunch(request);if(!engine.rootObjects().isEmpty())if(auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first())){window->show();window->raise();window->requestActivate();}};
  QObject::connect(&instance,&platform::Instance::request,&controller,dispatch);
  platform::OpenEvents openEvents;app.installEventFilter(&openEvents);
  QObject::connect(&openEvents,&platform::OpenEvents::open,&controller,[&](QString path){dispatch({{"version",1},{"action","open"},{"paths",QJsonArray{path}}});});
  if(!app.arguments().contains("--design-preview")&&startupFailure.isEmpty())QTimer::singleShot(0,&controller,[&]{dispatch(launch);});
  const auto scene=arguments.indexOf("--design-scene");
  if(testing&&scene>=0&&scene+1<arguments.size())QTimer::singleShot(300,&app,[&]{if(!engine.rootObjects().isEmpty())QMetaObject::invokeMethod(engine.rootObjects().first(),"openDesignScene",Q_ARG(QVariant,arguments[scene+1]));});
  QElapsedTimer smokeElapsed;smokeElapsed.start();QTimer smokeTimer;
  if(app.arguments().contains("--smoke")){smokeTimer.setInterval(100);QObject::connect(&smokeTimer,&QTimer::timeout,&app,[&]{
    if(smokeElapsed.elapsed()<700)return;
    if(checkUpdateSmoke){if(updates->state()=="idle"||updates->state()=="checking")return;if(updates->state()=="error"){app.exit(8);return;}}
    if(arguments.contains("--performance")&&startup.elapsed()<qMax<qint64>(firstFrame,archiveReady)+2000)return;
    if(!arguments.contains("--design-preview")&&(controller.busy()||(!launchPaths.isEmpty()&&controller.archiveFormat().isEmpty()))){if(smokeElapsed.elapsed()<15000)return;app.exit(4);return;}
    smokeTimer.stop();if(engine.rootObjects().isEmpty()){app.exit(1);return;}
    const auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    const auto index=app.arguments().indexOf("--screenshot");
    if(index>=0 && index+1<app.arguments().size() && (!window||!window->grabWindow().save(app.arguments()[index+1]))){app.exit(2);return;}
    const auto performance=arguments.indexOf("--performance");if(performance>=0&&performance+1<arguments.size()){
      QSaveFile file(arguments[performance+1]);if(!file.open(QIODevice::WriteOnly)){app.exit(5);return;}
      QJsonObject record{{"version",app.applicationVersion()},{"qt",qVersion()},{"firstFrameMs",firstFrame},{"archiveReadyMs",archiveReady},{"residentBytes",QString::number(platform::residentBytes())},{"measuredAtMs",startup.elapsed()},{"scene",launchPaths.isEmpty()?"empty":"archive"}};file.write(QJsonDocument(record).toJson());if(!file.commit()){app.exit(5);return;}
    }
    app.exit(0);
  });smokeTimer.start();}
  return app.exec();
}
