#include "../../models/EntryModel.hpp"
#include "../../models/ArchiveMetrics.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);EntryModel model;QJsonArray entries;
  const QJsonArray ratioEntries{QJsonObject{{"size","1000"},{"directory",false}},QJsonObject{{"size","99999"},{"directory",true}}};
  if(archiveMetrics::ratio(ratioEntries,600,true)!=0.6 || archiveMetrics::ratio(ratioEntries,1200,true)!=1.2 || archiveMetrics::ratio(ratioEntries,600,false)!=-1)return 7;
  if(archiveMetrics::ratio({},12,true)!=-1 || archiveMetrics::ratio(QJsonArray{QJsonObject{{"size","0"}}},12,true)!=-1 || archiveMetrics::ratio(QJsonArray{QJsonObject{{"size","-1"}}},12,true)!=-1)return 8;
  const QJsonArray large{QJsonObject{{"size","18446744073709551615"}},QJsonObject{{"size","18446744073709551615"}}};
  if(qAbs(archiveMetrics::ratio(large,18446744073709551615ULL,true)-0.5)>1e-12)return 9;
  if(archiveMetrics::ratio(QJsonArray{QJsonObject{}},600,true)!=-1 || archiveMetrics::ratio(ratioEntries,0,true)!=-1)return 10;
  for(int i=0;i<100000;++i)entries.append(QJsonObject{{"entryId","r:"+QString::number(i)},{"path","folder"+QString::number(i/1000)+"/file"+QString::number(i)+".txt"},{"size",QString::number(5000000000ULL+i)},{"directory",false}});
  QElapsedTimer time;time.start();model.load(entries,"r");if(model.rowCount()!=100)return 1;
  const auto id=model.data(model.index(0),EntryModel::Id).toString();if(!id.startsWith("r:dir:")||!model.enter(id)||model.rowCount()!=1000)return 2;
  model.filter("file99999");if(model.rowCount()!=0)return 3;model.parentFolder();model.filter("file99999");if(model.rowCount()!=1)return 4;
  if(!model.data(model.index(0),EntryModel::Size).toString().contains("GB"))return 5;
  model.select(model.data(model.index(0),EntryModel::Id).toString(),false);model.load(entries,"new");if(!model.selection().isEmpty())return 6;
  std::cout<<"100000 entries directory/filter/64-bit-size/selection reset PASS "<<time.elapsed()<<"ms\n";return 0;
}
