#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>

// A manual stable-release check; never downloads or executes installers.
class UpdateChecker : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
 public:
  explicit UpdateChecker(QString currentVersion,QObject* parent=nullptr,QNetworkAccessManager* transport=nullptr);
  QString state()const{return status;}
  QString latestVersion()const{return latest;}
  Q_INVOKABLE void check();
  Q_INVOKABLE bool openDownloadPage();
 signals:
  void changed();
 private:
  QString current,status{"idle"},latest;
  QNetworkAccessManager* network;
  QPointer<QNetworkReply> active;
};
