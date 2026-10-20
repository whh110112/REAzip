#include "../../app/AppController.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <iostream>
int main(int argc,char** argv){
  QCoreApplication app(argc,argv);if(app.arguments().size()!=4)return 2;
  QTemporaryDir work;AppController controller(app.arguments()[1],app.arguments()[2],nullptr,work.path()+"/data");int stage=0,passwordAttempts=0,overwrites=0;QString oldId;bool failed=false;
  auto require=[&](bool value,const char* message){if(!value){std::cerr<<message<<'\n';failed=true;app.exit(1);}return value;};
  auto model=qobject_cast<EntryModel*>(controller.entries());
  if(!require(controller.archiveRatio()==-1,"empty window must retain default logo"))return 1;
  const auto name=QString::fromUtf8("中文 😀.txt"),target=QDir(work.path()).filePath(name);
  auto read=[&](QString path){QFile f(path);f.open(QIODevice::ReadOnly);return f.readAll();};
  auto write=[&](QString path,QByteArray bytes){QFile f(path);if(f.open(QIODevice::WriteOnly))f.write(bytes);};
  std::function<void(std::function<void()>)> idle;
  idle=[&](std::function<void()> action){if(controller.busy())QTimer::singleShot(10,&app,[&,action]{idle(action);});else if(!failed)action();};
  auto extract=[&]{controller.extractArchive({{"destination",work.path()},{"smart",false},{"overwrite","ask"},{"openAfter",false}});};
  QObject::connect(&controller,&AppController::challenge,&app,[&](QString job,QString id,QString kind,QJsonObject){
    if(stage==5){controller.cancelTask();return;}
    if(kind=="passwordRequired")controller.answerChallenge(job,id,++passwordAttempts==1?"wrong":"test-only");
    else {++overwrites;controller.answerChallenge(job,id,stage==2?"skip":stage==3?"rename":"overwrite");}
  });
  QObject::connect(&controller,&AppController::operationFailed,&app,[&](QString code,QString message){
    if(stage==5&&code=="Cancelled"){stage=6;idle([&]{write(QDir(work.path()).filePath("source.txt"),"source");controller.createArchive({{"name","app.zip"},{"destination",work.path()},{"inputs",QVariantList{QUrl::fromLocalFile(QDir(work.path()).filePath("source.txt"))}},{"format","zip"},{"level",5}});});}
    else {std::cerr<<"stage "<<stage<<" "<<code.toStdString()<<" "<<message.toStdString()<<'\n';app.exit(1);}
  });
  QObject::connect(&controller,&AppController::operationCompleted,&app,[&](QString op,QJsonObject payload){
    if(op=="list"){
      long double original=0;for(const auto& value:payload["entries"].toArray()){const auto e=value.toObject();if(!e["directory"].toBool())original+=e["size"].toString().toULongLong();}
      const auto expected=static_cast<double>(payload["physicalSize"].toString().toULongLong()/original);
      if(!require(payload["physicalSizeComplete"].toBool()&&qAbs(controller.archiveRatio()-expected)<1e-12,"real worker listing ratio mismatch"))return;
      controller.setFilter("no-such-entry");if(!require(qAbs(controller.archiveRatio()-expected)<1e-12,"filter changed archive ratio"))return;controller.setFilter("");
    }
    if(stage==0&&op=="identity"){stage=1;idle([&]{controller.openUrls({QUrl::fromLocalFile(app.arguments()[3])});});}
    else if(stage==1&&op=="list"){
      if(!require(model->rowCount()==1&&passwordAttempts==2,"password retry/list failed"))return;
      oldId=model->data(model->index(0),EntryModel::Id).toString();controller.selectEntry(oldId,false);write(target,"keep");stage=2;idle(extract);
    }else if(stage==2&&op=="extract"){
      if(!require(read(target)=="keep"&&overwrites==1,"selected skip overwrote target"))return;stage=3;idle(extract);
    }else if(stage==3&&op=="extract"){
      if(!require(read(target)=="keep"&&overwrites==2&&QDir(work.path()).entryList(QDir::Files).size()==2,"selected rename failed"))return;stage=4;idle(extract);
    }else if(stage==4&&op=="extract"){
      if(!require(read(target)=="test"&&overwrites==3,"selected overwrite failed"))return;stage=5;idle([&]{controller.openUrls({QUrl::fromLocalFile(app.arguments()[3])});});
    }else if(stage==6&&op=="create"){stage=7;}
    else if(stage==7&&op=="list"){
      if(!require(controller.archiveFormat()=="zip"&&model->rowCount()==1,"created archive did not open"))return;
      stage=8;idle([&]{controller.selectEntry(model->data(model->index(0),EntryModel::Id).toString(),false);controller.renameSelected("renamed.txt");});
    }else if(stage==8&&op=="list"){
      if(!require(model->data(model->index(0),EntryModel::Name)=="renamed.txt","rename did not refresh model"))return;
      stage=9;idle([&]{write(QDir(work.path()).filePath("added.txt"),"added");controller.addUrls({QUrl::fromLocalFile(QDir(work.path()).filePath("added.txt"))});});
    }else if(stage==9&&op=="list"){
      if(!require(model->rowCount()==2,"add failed"))return;stage=10;idle([&]{controller.selectEntry(model->data(model->index(0),EntryModel::Id).toString(),false);controller.deleteSelected();});
    }else if(stage==10&&op=="list"){
      if(!require(model->rowCount()==1&&!controller.hasSelection(),"delete/selection invalidation failed"))return;stage=11;idle([&]{controller.testArchive();});
    }else if(stage==11&&op=="test"){std::cout<<"AppController/model/worker/native password, selected conflicts, cancellation, create/edit/test PASS\n";app.exit(0);}
  });
  QTimer::singleShot(45000,&app,[&]{std::cerr<<"integration timeout at stage "<<stage<<'\n';app.exit(1);});return app.exec();
}
