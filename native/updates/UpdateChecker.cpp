#include "UpdateChecker.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QTimer>
#include <memory>

namespace {
constexpr qint64 maximumResponse=1024*1024;
QVersionNumber stable(QString value){
  static const QRegularExpression pattern("^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$");
  if(!pattern.match(value).hasMatch())return {};
  const auto version=QVersionNumber::fromString(value);
  return version.segmentCount()==3?version:QVersionNumber{};
}
}
UpdateChecker::UpdateChecker(QString version,QObject* parent,QNetworkAccessManager* transport)
    :QObject(parent),current(std::move(version)),network(transport?transport:new QNetworkAccessManager(this)){}
void UpdateChecker::check(){
  if(active)return;
  latest.clear();status="checking";emit changed();
  QNetworkRequest request(QUrl("https://api.github.com/repos/whh110112/ZENzip/releases/latest"));
  request.setRawHeader("Accept","application/vnd.github+json");
  request.setRawHeader("User-Agent","ZENzip-update-check");
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
  request.setTransferTimeout(15000);
  auto reply=network->get(request);active=reply;
  auto payload=std::make_shared<QByteArray>();
  auto tooLarge=std::make_shared<bool>(false);
  auto read=[reply,payload,tooLarge]{
    if(*tooLarge||!reply->isOpen()||!reply->bytesAvailable())return;
    payload->append(reply->read(maximumResponse-payload->size()+1));
    if(payload->size()>maximumResponse){*tooLarge=true;reply->abort();}
  };
  connect(reply,&QNetworkReply::readyRead,this,read);
  connect(reply,&QNetworkReply::finished,this,[this,reply,payload,tooLarge,read]{
    if(active!=reply){reply->deleteLater();return;}
    // Stop the timeout and any duplicate finished delivery before parsing.
    active.clear();
    read();
    status="error";
    if(!*tooLarge&&reply->error()==QNetworkReply::NoError&&reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200){
      QJsonParseError error;
      const auto document=QJsonDocument::fromJson(*payload,&error);
      const auto release=document.object();
      const auto tag=release.value("tag_name").toString();
      const auto version=tag.startsWith('v')?tag.mid(1):QString();
      const auto remote=stable(version),local=stable(current);
      // Keep the browser target on our own canonical stable release page.
      const auto page=QString("https://github.com/whh110112/ZENzip/releases/tag/")+tag;
      if(error.error==QJsonParseError::NoError&&document.isObject()&&
         release.value("draft").isBool()&&!release.value("draft").toBool()&&
         release.value("prerelease").isBool()&&!release.value("prerelease").toBool()&&
         !remote.isNull()&&!local.isNull()&&release.value("html_url").toString()==page){
        latest=version;status=QVersionNumber::compare(remote,local)>0?"available":"current";
      }
    }
    reply->deleteLater();emit changed();
  });
  QTimer::singleShot(15000,reply,[this,reply]{if(active==reply)reply->abort();});
}
bool UpdateChecker::openDownloadPage(){
  if(status!="available"||stable(latest).isNull())return false;
  return QDesktopServices::openUrl(QUrl("https://github.com/whh110112/ZENzip/releases/tag/v"+latest));
}
