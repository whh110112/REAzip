#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
class EntryModel : public QAbstractListModel {
  Q_OBJECT
 public:
  enum Role { Id=Qt::UserRole+1,Name,Directory,Size,Packed,Kind,Modified,Selected };
  explicit EntryModel(QObject* parent=nullptr):QAbstractListModel(parent){}
  int rowCount(const QModelIndex& parent={})const override{return parent.isValid()?0:rows.size();}
  QVariant data(const QModelIndex&,int)const override;
  QHash<int,QByteArray> roleNames()const override;
  void load(QJsonArray entries,QString revision,QString folder={});
  void filter(QString value);void sort(int column);void select(QString id,bool additive);
  bool enter(QString id);void parentFolder();
  QString folder()const{return currentFolder;}
  QStringList selection()const{return selected.values();}
  QString selectedName()const;QJsonObject entry(QString id)const;
  const QJsonArray& all()const{return source;}
  static QString bytes(quint64);
 signals:void changed();
 private:
  QJsonArray source;QVector<QJsonObject> rows;QSet<QString> selected;QString revision,currentFolder,query;int column{};bool ascending{true};
  void rebuild();
};
