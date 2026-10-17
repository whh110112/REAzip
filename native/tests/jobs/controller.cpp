#include "JobController.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <QFileInfo>
#include <iostream>
int main(int argc,char** argv){
  QCoreApplication app(argc,argv);if(app.arguments().size()!=5)return 2;
  JobController jobs(app.arguments()[1],app.arguments()[2]);int completed=0;QString cancelId;
  QObject::connect(&jobs,&JobController::challenge,&app,[&](QString job,QString challenge,QString kind,QJsonObject){
    if(job==cancelId){jobs.cancel();return;}
    if(kind=="passwordRequired")jobs.respond(job,challenge,"test-only");else {std::cerr<<"unexpected overwrite\n";app.exit(1);}
  });
  QObject::connect(&jobs,&JobController::result,&app,[&](QString,QString,QJsonObject data){
    if(data["entries"].toArray().size()!=1){app.exit(1);return;}
    if(++completed==2)cancelId=jobs.submit({{"operation","list"},{"archive",app.arguments()[4]}});
  });
  QObject::connect(&jobs,&JobController::failed,&app,[&](QString id,QString code,QString){
    if(id==cancelId&&code=="Cancelled"&&completed==2){std::cout<<"controller queue/challenge/cancel PASS\n";app.exit(0);}else app.exit(1);
  });
  jobs.submit({{"operation","list"},{"archive",app.arguments()[3]}});
  jobs.submit({{"operation","list"},{"archive",app.arguments()[4]}});
  QTimer::singleShot(25000,&app,[&]{std::cerr<<"controller timeout\n";app.exit(1);});return app.exec();
}
