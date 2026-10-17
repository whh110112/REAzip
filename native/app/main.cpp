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
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setOrganizationName("REAzip");
  QGuiApplication::setApplicationName("REAzip");
  QQuickStyle::setStyle("Basic");
  const auto arguments=app.arguments();
  if(arguments.contains("--register"))return platform::registerApplication(app.applicationFilePath()).isEmpty()?0:2;
  if(arguments.contains("--unregister"))return platform::unregisterApplication(app.applicationFilePath()).isEmpty()?0:2;
  QJsonArray launchPaths;QString action="open";
  for(int i=1;i<arguments.size();++i){const auto arg=arguments[i];if(arg=="--screenshot"||arg=="--design-scene"||arg=="--theme"||arg=="--language"){++i;continue;}if(arg=="--extract-here")action="extract_here";else if(arg=="--extract-to")action="extract_to";else if(arg=="--add")action="add";else if(!arg.startsWith('-')&&QFileInfo::exists(arg))launchPaths.append(QFileInfo(arg).absoluteFilePath());}
  QJsonObject launch{{"version",1},{"action",action},{"paths",launchPaths}};
  platform::Instance instance(platform::dataDirectory());
  if(!arguments.contains("--smoke")&&!arguments.contains("--design-preview")){const auto state=instance.start(launch);if(state==1)return 0;if(state<0){qCritical().noquote()<<instance.error();return 3;}}
#ifdef Q_OS_WIN
  const auto worker=QCoreApplication::applicationDirPath()+"/reazip-worker.exe";
  const auto library=QCoreApplication::applicationDirPath()+"/engine/7z.dll";
#else
  const auto worker=QCoreApplication::applicationDirPath()+"/reazip-worker";
  const auto library=QCoreApplication::applicationDirPath()+"/engine/7z.so";
#endif
  AppController controller(worker,library);
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("backend",app.arguments().contains("--design-preview")?nullptr:static_cast<QObject*>(&controller));
  engine.rootContext()->setContextProperty("designPreview",app.arguments().contains("--design-preview"));
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule("REAzip", "Main");
  auto dispatch=[&](QJsonObject request){controller.handleLaunch(request);if(!engine.rootObjects().isEmpty())if(auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first())){window->show();window->raise();window->requestActivate();}};
  QObject::connect(&instance,&platform::Instance::request,&controller,dispatch);
  platform::OpenEvents openEvents;app.installEventFilter(&openEvents);
  QObject::connect(&openEvents,&platform::OpenEvents::open,&controller,[&](QString path){dispatch({{"version",1},{"action","open"},{"paths",QJsonArray{path}}});});
  if(!app.arguments().contains("--design-preview"))QTimer::singleShot(0,&controller,[&]{dispatch(launch);});
  if(app.arguments().contains("--smoke")) QTimer::singleShot(700,&app,[&]{
    if(engine.rootObjects().isEmpty()){app.exit(1);return;}
    const auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    const auto index=app.arguments().indexOf("--screenshot");
    if(index>=0 && index+1<app.arguments().size() && (!window||!window->grabWindow().save(app.arguments()[index+1]))){app.exit(2);return;}
    app.exit(0);
  });
  return app.exec();
}
