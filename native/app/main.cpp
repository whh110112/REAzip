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
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setOrganizationName("REAzip");
  QGuiApplication::setApplicationName("REAzip");
  QQuickStyle::setStyle("Basic");
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
  QVariantList paths;
  for(const auto& arg:app.arguments().mid(1))if(QFileInfo(arg).isFile())paths.append(QUrl::fromLocalFile(QFileInfo(arg).absoluteFilePath()));
  if(!app.arguments().contains("--design-preview"))controller.openUrls(paths);
  if(app.arguments().contains("--smoke")) QTimer::singleShot(700,&app,[&]{
    if(engine.rootObjects().isEmpty()){app.exit(1);return;}
    const auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    const auto index=app.arguments().indexOf("--screenshot");
    if(index>=0 && index+1<app.arguments().size() && (!window||!window->grabWindow().save(app.arguments()[index+1]))){app.exit(2);return;}
    app.exit(0);
  });
  return app.exec();
}
