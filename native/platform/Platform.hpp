#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLockFile>
#include <QJsonObject>
#include <memory>
namespace platform {
QString dataDirectory(QString applicationDirectory={});
QString registerApplication(QString executable,QString testRegistryPrefix={});
QString unregisterApplication(QString executable,QString testRegistryPrefix={});
bool propagateOrigin(QString archive,QString destination,QString* error=nullptr);
quint64 residentBytes();
class Instance : public QObject {
  Q_OBJECT
 public:
  explicit Instance(QString directory,QObject* parent=nullptr);
  // 0 primary, 1 forwarded, -1 error; only primary removes stale socket.
  int start(QJsonObject request);QString error()const{return diagnostic;}
  static bool valid(const QJsonObject& request);
 signals:void request(QJsonObject data);
 private:QLocalServer server;QString name,diagnostic,storage;std::unique_ptr<QLockFile> lock;
};
class OpenEvents : public QObject {
  Q_OBJECT
 public:using QObject::QObject;
 signals:void open(QString path);
 protected:bool eventFilter(QObject*,QEvent*)override;
};
}
