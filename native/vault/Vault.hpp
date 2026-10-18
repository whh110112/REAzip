#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QJsonArray>
#include <QVector>
// Password vault.
//
// On Windows the entry list is encrypted with CryptProtectData (DPAPI), i.e. it can only be read by
// the same Windows account on the same machine - no master password, no key material we have to
// store. On macOS/Linux there is no such built-in per-user secret store available without an extra
// runtime dependency, so the file is written with owner-only permissions and the UI reports the
// weaker protection verbatim instead of pretending it is encrypted.
class Vault : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
  Q_PROPERTY(QString protection READ protection NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
 public:
  explicit Vault(QString directory,QObject* parent=nullptr);
  bool load();
  QVariantList entries()const;
  QString protection()const{return mode;}
  QString error()const{return diagnostic;}
  // Callers must not log the secret. Returns the stored secret for the given entry id ("" if absent).
  Q_INVOKABLE QString secret(const QString& id)const;
  Q_INVOKABLE bool add(const QString& name,const QString& value);
  Q_INVOKABLE bool remove(const QString& id);
  // Records that the entry was used, so the UI can show recent first.
  Q_INVOKABLE void touch(const QString& id);
 signals:
  void changed();
 private:
  struct Entry {QString id,name,value,updated;qint64 stamp{};};
  QString root,mode,diagnostic;
  QVector<Entry> items;
  // Set when the file exists in a form this build cannot read (e.g. a DPAPI vault opened on Linux).
  // Writing would destroy it, so every mutation is refused instead.
  bool readOnly{};
  bool save();
  void report(const QString& text);
};
