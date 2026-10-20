#pragma once
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QJsonArray>
#include <QJsonObject>
class JobController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(double progress READ progress NOTIFY changed)
  Q_PROPERTY(QString state READ state NOTIFY changed)
 public:
  explicit JobController(QString worker,QString engine,QObject* parent=nullptr);
  ~JobController() override;
  QString submit(QJsonObject request);
  bool busy()const{return active!=nullptr;}
  double progress()const{return percent;}
  QString state()const{return phase;}
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void respond(const QString& jobId,const QString& challengeId,const QString& value,bool applyToAll=false);
 signals:
  void changed();
  void challenge(QString jobId,QString challengeId,QString kind,QJsonObject details);
  void result(QString jobId,QString operation,QJsonObject payload);
  void failed(QString jobId,QString code,QString message);
 private:
  QString workerPath,enginePath,activeId,operation,challengeId,challengeKind,phase;
  QQueue<QJsonObject> pending;
  QProcess* active{};QByteArray incoming;QJsonArray entries;double percent{};bool terminal{};bool committing{};
  void startNext();void write(QJsonObject);void read();void fail(QString,QString);void cancellationTimeout();
};
