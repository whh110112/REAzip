#include "../../models/EntryModel.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);EntryModel model;QJsonArray entries;
  for(int i=0;i<100000;++i)entries.append(QJsonObject{{"entryId","r:"+QString::number(i)},{"path","folder"+QString::number(i/1000)+"/file"+QString::number(i)+".txt"},{"size",QString::number(5000000000ULL+i)},{"directory",false}});
  QElapsedTimer time;time.start();model.load(entries,"r");if(model.rowCount()!=100)return 1;
  const auto id=model.data(model.index(0),EntryModel::Id).toString();if(!id.startsWith("r:dir:")||!model.enter(id)||model.rowCount()!=1000)return 2;
  model.filter("file99999");if(model.rowCount()!=0)return 3;model.parentFolder();model.filter("file99999");if(model.rowCount()!=1)return 4;
  if(!model.data(model.index(0),EntryModel::Size).toString().contains("GB"))return 5;
  model.select(model.data(model.index(0),EntryModel::Id).toString(),false);model.load(entries,"new");if(!model.selection().isEmpty())return 6;
  std::cout<<"100000 entries directory/filter/64-bit-size/selection reset PASS "<<time.elapsed()<<"ms\n";return 0;
}
