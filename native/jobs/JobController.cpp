#include "JobController.hpp"
#include <QJsonDocument>
#include <QUuid>
#include <QTimer>
#include <QFileInfo>
JobController::JobController(QString worker,QString engine,QObject* parent):QObject(parent),workerPath(std::move(worker)),enginePath(std::move(engine)){}
JobController::~JobController(){if(active){active->closeWriteChannel();if(!active->waitForFinished(2000)&&!committing){active->kill();active->waitForFinished(2000);}else if(committing)active->waitForFinished(-1);}}
QString JobController::submit(QJsonObject request){
  const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);request["jobId"]=id;request["protocolVersion"]=1;request["type"]="submit";pending.enqueue(request);if(!active)startNext();return id;
}
void JobController::write(QJsonObject frame){if(active){frame["protocolVersion"]=1;frame["jobId"]=activeId;active->write(QJsonDocument(frame).toJson(QJsonDocument::Compact)+'\n');}}
void JobController::startNext(){
  if(active||pending.isEmpty())return;
  const auto request=pending.dequeue();activeId=request["jobId"].toString();operation=request["operation"].toString();incoming.clear();entries={};terminal=false;committing=false;percent=0;phase="Running";challengeId.clear();
  active=new QProcess(this);active->setProgram(workerPath);active->setArguments({"--engine",enginePath});active->setProcessChannelMode(QProcess::SeparateChannels);
  connect(active,&QProcess::started,this,[this,request]{write(request);});
  connect(active,&QProcess::readyReadStandardOutput,this,&JobController::read);
  connect(active,&QProcess::readyReadStandardError,this,[this]{active->readAllStandardError();});
  connect(active,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart){fail("WorkerMissing","Native worker could not start");auto old=active;active=nullptr;old->deleteLater();emit changed();QTimer::singleShot(0,this,&JobController::startNext);}});
  connect(active,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int,QProcess::ExitStatus){read();if(!terminal)fail("WorkerCrashed","Native worker exited before a result");auto old=active;active=nullptr;old->deleteLater();phase="Idle";emit changed();QTimer::singleShot(0,this,&JobController::startNext);});
  active->start();emit changed();
}
void JobController::fail(QString code,QString message){if(terminal)return;terminal=true;phase=code=="Cancelled"?"Cancelled":"Failed";emit failed(activeId,code,message);emit changed();}
void JobController::read(){
  if(!active)return;incoming+=active->readAllStandardOutput();
  if(incoming.size()>16*1024*1024 && !incoming.contains('\n')){fail("ProtocolError","Worker frame exceeded limit");active->kill();return;}
  for(;;){auto end=incoming.indexOf('\n');if(end<0)break;auto data=incoming.left(end);incoming.remove(0,end+1);
    if(data.size()>16*1024*1024){fail("ProtocolError","Worker frame exceeded limit");active->kill();break;}
    QJsonParseError parse;const auto doc=QJsonDocument::fromJson(data,&parse);
    if(parse.error!=QJsonParseError::NoError||!doc.isObject()){fail("ProtocolError","Invalid worker frame");active->kill();break;}
    const auto frame=doc.object();if(frame["protocolVersion"].toInt()!=1){fail("ProtocolError","Worker version mismatch");active->kill();break;}
    const auto type=frame["type"].toString();if(type=="ready")continue;if(frame["jobId"].toString()!=activeId||terminal)continue;
    if(type=="entriesChunk"){for(const auto& item:frame["entries"].toArray())entries.append(item);}
    else if(type=="progress"){
      if(frame.contains("phase")){committing=frame["phase"].toString()=="committing";phase=committing?"Committing":"Running";}
      else{const double total=frame["total"].toString().toDouble();if(total>0)percent=qBound(0.0,100.0*frame["current"].toString().toDouble()/total,100.0);}
      emit changed();
    }else if(type=="passwordRequired"||type=="overwriteRequired"){
      challengeId=frame["challengeId"].toString();challengeKind=type;phase=type=="passwordRequired"?"AwaitingPassword":"AwaitingOverwrite";emit changed();emit challenge(activeId,challengeId,type,frame);
    }else if(type=="completed"){
      terminal=true;phase="Succeeded";percent=100;auto payload=frame;if(operation=="list")payload["entries"]=entries;emit result(activeId,operation,payload);active->closeWriteChannel();emit changed();
    }else if(type=="failed"||type=="cancelled"){
      fail(type=="cancelled"?"Cancelled":frame["code"].toString(),frame["message"].toString());active->closeWriteChannel();
    }
  }
}
void JobController::respond(const QString& jobId,const QString& id,const QString& value){
  if(!active||terminal||jobId!=activeId||id!=challengeId)return;
  QJsonObject response{{"type","respondChallenge"},{"challengeId",id}};
  response[challengeKind=="passwordRequired"?"password":"choice"]=value;write(response);challengeId.clear();phase="Running";emit changed();
}
void JobController::cancel(){if(!active||terminal)return;write({{"type","cancel"}});phase="Cancelling";emit changed();const auto id=activeId;QTimer::singleShot(5000,this,[this,id]{if(active&&activeId==id&&!terminal)cancellationTimeout();});}
void JobController::cancellationTimeout(){if(!active||terminal)return;if(committing){const auto id=activeId;QTimer::singleShot(1000,this,[this,id]{if(active&&activeId==id)cancellationTimeout();});return;}active->kill();fail("Cancelled","Task process was stopped after cancellation timeout");}
