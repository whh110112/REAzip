#include "../../updates/UpdateChecker.hpp"
#include <QCoreApplication>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QTest>
#include <cstring>

class Reply : public QNetworkReply {
  QByteArray payload;qint64 offset{};bool completed{};
 public:
  Reply(QNetworkRequest request,QByteArray data,int status,NetworkError failure,QObject* parent):QNetworkReply(parent),payload(std::move(data)){
    setRequest(request);setUrl(request.url());setOperation(QNetworkAccessManager::GetOperation);
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);open(QIODevice::ReadOnly);
    if(failure!=NoError)setError(failure,"test transport failure");
    QTimer::singleShot(0,this,[this]{if(completed)return;emit readyRead();if(!completed){completed=true;setFinished(true);emit finished();}});
  }
  void abort()override{if(completed)return;completed=true;setError(OperationCanceledError,"cancelled");setFinished(true);emit finished();}
  qint64 bytesAvailable()const override{return payload.size()-offset+QNetworkReply::bytesAvailable();}
  bool isSequential()const override{return true;}
 protected:
  qint64 readData(char* out,qint64 count)override{
    const auto n=qMin(count,qint64(payload.size()-offset));if(n<=0)return -1;
    std::memcpy(out,payload.constData()+offset,size_t(n));offset+=n;return n;
  }
};
class Transport : public QNetworkAccessManager {
 public:
  QByteArray payload;int status{200},requests{};QNetworkReply::NetworkError failure{QNetworkReply::NoError};QUrl requested;
 protected:
  QNetworkReply* createRequest(Operation,QNetworkRequest const& request,QIODevice*)override{
    ++requests;requested=request.url();return new Reply(request,payload,status,failure,this);
  }
};
class UpdateTests : public QObject {
  Q_OBJECT
 private slots:
  void stableChecks_data(){
    QTest::addColumn<QString>("version");QTest::addColumn<QString>("tag");QTest::addColumn<QString>("result");
    QTest::newRow("newer")<<"0.4.2"<<"v0.4.3"<<"available";
    QTest::newRow("same")<<"0.4.2"<<"v0.4.2"<<"current";
    QTest::newRow("local-newer")<<"0.4.3"<<"v0.4.2"<<"current";
    QTest::newRow("numeric-order")<<"0.9.0"<<"v0.10.0"<<"available";
    QTest::newRow("dev-tag")<<"0.4.2"<<"v0.5.0-dev"<<"error";
    QTest::newRow("malformed")<<"0.4.2"<<"vno-version"<<"error";
  }
  void stableChecks(){
    QFETCH(QString,version);QFETCH(QString,tag);QFETCH(QString,result);
    Transport transport;
    transport.payload=QJsonDocument(QJsonObject{{"tag_name",tag},{"draft",false},{"prerelease",false},{"html_url","https://github.com/whh110112/ZENzip/releases/tag/"+tag}}).toJson();
    UpdateChecker checker(version,nullptr,&transport);QCOMPARE(transport.requests,0);QCOMPARE(checker.state(),QString("idle"));
    QVERIFY(!checker.openDownloadPage());checker.check();checker.check();QCOMPARE(transport.requests,1);
    QTRY_COMPARE(checker.state(),result);
    QCOMPARE(transport.requested,QUrl("https://api.github.com/repos/whh110112/ZENzip/releases/latest"));
    if(result=="error")QVERIFY(checker.latestVersion().isEmpty());else QCOMPARE(checker.latestVersion(),tag.mid(1));
  }
  void failuresAndRetry(){
    Transport transport;UpdateChecker checker("0.4.2",nullptr,&transport);
    const auto release=QJsonObject{{"tag_name","v0.4.3"},{"draft",false},{"prerelease",false},{"html_url","https://github.com/whh110112/ZENzip/releases/tag/v0.4.3"}};
    auto check=[&]{checker.check();};
    for(const auto failure:{QNetworkReply::TimeoutError,QNetworkReply::SslHandshakeFailedError,QNetworkReply::HostNotFoundError}){
      transport.failure=failure;check();QTRY_COMPARE(checker.state(),QString("error"));
    }
    transport.failure=QNetworkReply::NoError;
    for(int code:{403,404,429,500,302}){transport.status=code;check();QTRY_COMPARE(checker.state(),QString("error"));}
    transport.status=200;
    for(const QByteArray& payload:{QByteArray("{bad json"),QByteArray(1024*1024+1,'x')}){
      transport.payload=payload;check();QTRY_COMPARE(checker.state(),QString("error"));
    }
    for(const auto& field:{QString("draft"),QString("prerelease"),QString("html_url")}){
      auto data=release;data[field]=field=="html_url"?QJsonValue("https://example.com/installer"):QJsonValue(true);
      transport.payload=QJsonDocument(data).toJson();check();QTRY_COMPARE(checker.state(),QString("error"));
    }
    transport.payload=QJsonDocument(release).toJson();check();QTRY_COMPARE(checker.state(),QString("available"));
    // A failed recheck must clear an earlier positive result and download target.
    transport.failure=QNetworkReply::HostNotFoundError;check();QTRY_COMPARE(checker.state(),QString("error"));QVERIFY(checker.latestVersion().isEmpty());QVERIFY(!checker.openDownloadPage());
  }
};
QTEST_GUILESS_MAIN(UpdateTests)
#include "updates.moc"
