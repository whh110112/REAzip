#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QImage>
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setOrganizationName("REAzip");
  QGuiApplication::setApplicationName("REAzip");
  QQmlApplicationEngine engine;
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule("REAzip", "Main");
  if(app.arguments().contains("--smoke")) QTimer::singleShot(700,&app,[&]{
    if(engine.rootObjects().isEmpty()){app.exit(1);return;}
    const auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    const auto index=app.arguments().indexOf("--screenshot");
    if(index>=0 && index+1<app.arguments().size() && (!window||!window->grabWindow().save(app.arguments()[index+1]))){app.exit(2);return;}
    app.exit(0);
  });
  return app.exec();
}
