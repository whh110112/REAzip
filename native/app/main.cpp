#include <QGuiApplication>
#include <QQmlApplicationEngine>
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setOrganizationName("REAzip");
  QGuiApplication::setApplicationName("REAzip");
  QQmlApplicationEngine engine;
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule("REAzip", "Main");
  return app.exec();
}
