#pragma once
#include "../jobs/JobController.hpp"
#include "../models/EntryModel.hpp"
#include <QVariantList>
#include <QTemporaryDir>
#include <QUrl>
#include "../settings/Settings.hpp"
#include "../themes/Themes.hpp"
#include "../vault/Vault.hpp"
#include "../updates/UpdateChecker.hpp"
#include <memory>
#include <vector>
class AppController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QObject* entries READ entries CONSTANT)
  Q_PROPERTY(QVariantList tabs READ tabs NOTIFY changed)
  Q_PROPERTY(QString activeTab READ activeTab NOTIFY changed)
  Q_PROPERTY(bool hasArchive READ hasArchive NOTIFY changed)
  Q_PROPERTY(QString archiveName READ archiveName NOTIFY changed)
  Q_PROPERTY(QString archiveFormat READ archiveFormat NOTIFY changed)
  Q_PROPERTY(double archiveRatio READ archiveRatio NOTIFY changed)
  Q_PROPERTY(QString breadcrumb READ breadcrumb NOTIFY changed)
  Q_PROPERTY(QVariantList details READ details NOTIFY changed)
  Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY jobChanged)
  Q_PROPERTY(bool canModify READ canModify NOTIFY changed)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY changed)
  Q_PROPERTY(QString selectedName READ selectedName NOTIFY changed)
  Q_PROPERTY(QString defaultArchiveDir READ defaultArchiveDir NOTIFY changed)
  Q_PROPERTY(QString defaultExtractDir READ defaultExtractDir NOTIFY changed)
  Q_PROPERTY(QString extractDirMode READ extractDirMode NOTIFY changed)
  Q_PROPERTY(QString customExtractDir READ customExtractDir NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY jobChanged)
  Q_PROPERTY(double progress READ progress NOTIFY jobChanged)
  Q_PROPERTY(QString jobState READ jobState NOTIFY jobChanged)
  Q_PROPERTY(QString engineIdentity READ engineIdentity NOTIFY changed)
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
  Q_PROPERTY(QVariantMap themeBackground READ themeBackground NOTIFY changed)
  Q_PROPERTY(QVariantList themeList READ themeList NOTIFY changed)
  Q_PROPERTY(QVariantList languages READ languages NOTIFY changed)
  Q_PROPERTY(bool rtl READ rtl NOTIFY changed)
  Q_PROPERTY(QString languageCode READ languageCode NOTIFY changed)
  Q_PROPERTY(QString filenameCodepage READ codepage NOTIFY changed)
  Q_PROPERTY(QString themeId READ themeId NOTIFY changed)
  Q_PROPERTY(bool restoreTabs READ restoreTabs NOTIFY changed)
  Q_PROPERTY(QVariantList passwords READ passwords NOTIFY changed)
  Q_PROPERTY(QString vaultProtection READ vaultProtection NOTIFY changed)
  Q_PROPERTY(QString associationStatus READ associationStatus NOTIFY changed)
  Q_PROPERTY(int revision READ revision NOTIFY changed)
  Q_PROPERTY(bool dragPrepared READ dragPrepared NOTIFY changed)
  Q_PROPERTY(QObject* updates READ updates CONSTANT)
 public:
  AppController(QString worker,QString engine,QObject* parent=nullptr,QString dataDirectory={});
  ~AppController() override;
  QObject* entries(){return &model;} QVariantList tabs()const;QString activeTab()const{return current;}
  bool hasArchive()const{return tab()!=nullptr;}QString archiveName()const;QString archiveFormat()const;QString breadcrumb()const;
  double archiveRatio()const;
  QVariantList details()const;QString statusSummary()const;QString status()const;
  bool canModify()const;bool hasSelection()const{return !model.selection().isEmpty();}QString selectedName()const{return model.selectedName();}
  QString defaultArchiveDir()const;QString defaultExtractDir()const;
  QString extractDirMode()const{return preferences["extractDirMode"].toString("archive");}
  QString customExtractDir()const{return preferences["extractDir"].toString();}
  QString version()const;
  QObject* updates(){return &updateChecker;}
  bool busy()const{return jobs.busy();}double progress()const{return jobs.progress();}QString jobState()const{return jobs.state();}QString engineIdentity()const{return identity;}
  QVariantMap colors()const;QVariantList themeList()const;QVariantList languages()const;
  QVariantMap themeBackground()const;
  bool rtl()const;bool restoreTabs()const{return preferences["restoreTabs"].toBool(true);}int revision()const{return epoch;}
  QString languageCode()const{return preferences["language"].toString();}
  QString themeId()const{return preferences["skin"].toString();}
  QVariantList passwords()const{return vault.entries();}QString vaultProtection()const{return vault.protection();}
  QString associationStatus()const;
  Q_INVOKABLE QString translate(QString key,QString fallback)const;
  Q_INVOKABLE QString localPath(QUrl url)const;
  Q_INVOKABLE void openUrls(QVariantList urls);
  Q_INVOKABLE void closeTab(QString id);Q_INVOKABLE void activateTab(QString id);
  Q_INVOKABLE void testArchive();Q_INVOKABLE void setFilter(QString text){model.filter(text);}
  Q_INVOKABLE void parentFolder(){model.parentFolder();}
  Q_INVOKABLE void sortBy(int column){model.sort(column);}
  Q_INVOKABLE void selectEntry(QString id,bool additive){model.select(id,additive);}
  Q_INVOKABLE void activateEntry(QString id);
  Q_INVOKABLE void addUrls(QVariantList urls);
  Q_INVOKABLE void createArchive(QVariantMap options);Q_INVOKABLE void extractArchive(QVariantMap options);
  Q_INVOKABLE void answerChallenge(QString job,QString id,QString value,bool applyToAll=false){jobs.respond(job,id,value,applyToAll);}
  Q_INVOKABLE void cancelTask(){jobs.cancel();}
  Q_INVOKABLE void deleteSelected();Q_INVOKABLE void renameSelected(QString name);
  Q_INVOKABLE void setPreference(QString key,QVariant value);
  Q_INVOKABLE void openThemeDirectory();Q_INVOKABLE void rescanThemes();Q_INVOKABLE void applyAssociations();Q_INVOKABLE void refreshAssociations();
  // Password vault. Secrets never leave the process except into a challenge answer or a create request.
  Q_INVOKABLE QString passwordOf(QString id)const{return vault.secret(id);}
  Q_INVOKABLE bool savePassword(QString name,QString secret);
  Q_INVOKABLE bool forgetPassword(QString id);
  Q_INVOKABLE void usePassword(QString id){vault.touch(id);}
  Q_INVOKABLE void openExternal(QString path);
  Q_INVOKABLE void handleLaunch(QJsonObject request);
  Q_INVOKABLE void prepareDrag();Q_INVOKABLE void beginPreparedDrag();bool dragPrepared()const{return !dragUrls.isEmpty();}
 signals:
  void changed();void jobChanged();void message(QString text,bool error);
  void challenge(QString jobId,QString challengeId,QString kind,QJsonObject details);
  void executableRequested(QString path);
  void createRequested(QVariantList paths);void extractRequested();
  void operationCompleted(QString operation,QJsonObject result);void operationFailed(QString code,QString text);
 private:
  struct Tab {QString id,path,format,revision,folder;QJsonArray entries;quint64 physical{};double ratio{-1};};
  struct Context {QString tabId,path,openEntry;bool openAfter{},temporary{},drag{};};
  JobController jobs;EntryModel model;QVector<Tab> documents;QHash<QString,Context> contexts;QString current,identity;
  Settings settings;Vault vault;Themes themeService;QJsonObject preferences,translations,english;QJsonArray skins;int epoch{};
  mutable QString associationText;mutable bool associationDirty{true};
  QList<QUrl> dragUrls;QHash<QString,QString> launchActions;
  std::vector<std::unique_ptr<QTemporaryDir>> temporary;
  UpdateChecker updateChecker;
  const Tab* tab()const;Tab* find(QString id);void showTab();
  QString submit(QJsonObject request,Context context);void refresh(QString id);
  void completed(QString id,QString operation,QJsonObject payload);void failed(QString id,QString code,QString text);
  QJsonArray paths(QVariantList urls)const;QString codepage()const{return preferences["codepage"].toString();}
};
