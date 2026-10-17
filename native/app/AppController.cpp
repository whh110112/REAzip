#include "AppController.hpp"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUuid>
#include <QColor>
#include <QLocale>
#include <QUrl>
#include <QCoreApplication>
#include <QDrag>
#include <QMimeData>
#include "../platform/Platform.hpp"
namespace {
QJsonDocument json(QString path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};return QJsonDocument::fromJson(file.readAll());}
bool executable(QString name){static const QStringList extensions={"exe","com","bat","cmd","ps1","sh","bash","zsh","app","desktop","msi","scr","vbs","js","jse","wsf","hta","py","pl","rb","jar","lnk","url"};return extensions.contains(QFileInfo(name).suffix().toLower())||QFileInfo(name).isExecutable();}
}
AppController::AppController(QString worker,QString engine,QObject* parent):QObject(parent),jobs(worker,engine,this),model(this){
  preferences={{"skin","light"},{"language","zh-CN"},{"restoreTabs",true}};
  skins=json(":/data/themes/builtin.json").array();translations=json(":/data/i18n/zh-CN.json").object();
  connect(&model,&EntryModel::changed,this,[this]{dragUrls.clear();if(auto t=find(current))t->folder=model.folder();emit changed();});
  connect(&jobs,&JobController::changed,this,&AppController::jobChanged);
  connect(&jobs,&JobController::changed,this,&AppController::changed);
  connect(&jobs,&JobController::challenge,this,&AppController::challenge);
  connect(&jobs,&JobController::result,this,&AppController::completed);
  connect(&jobs,&JobController::failed,this,&AppController::failed);
  submit({{"operation","identity"}},{});
}
const AppController::Tab* AppController::tab()const{for(const auto& t:documents)if(t.id==current)return &t;return nullptr;}
AppController::Tab* AppController::find(QString id){for(auto& t:documents)if(t.id==id)return &t;return nullptr;}
QVariantList AppController::tabs()const{QVariantList result;for(const auto& t:documents)result.append(QVariantMap{{"id",t.id},{"name",QFileInfo(t.path).fileName()}});return result;}
QString AppController::archiveName()const{return tab()?QFileInfo(tab()->path).fileName():QString();}
QString AppController::archiveFormat()const{return tab()?tab()->format:QString();}
QString AppController::breadcrumb()const{return archiveName()+(model.folder().isEmpty()?QString():" / "+model.folder());}
bool AppController::canModify()const{return tab()&&!busy()&&(tab()->format=="zip"||tab()->format=="7z"||tab()->format=="tar");}
QString AppController::defaultArchiveDir()const{return tab()?QFileInfo(tab()->path).absolutePath():QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);}
QString AppController::defaultExtractDir()const{return defaultArchiveDir();}
QString AppController::status()const{return busy()?jobs.state():translate("ready",QString::fromUtf8("就绪"));}
QString AppController::statusSummary()const{if(!tab())return {};return QString::number(tab()->entries.size())+" "+translate("files",QString::fromUtf8("条目"))+" · "+EntryModel::bytes(tab()->physical);}
QVariantList AppController::details()const{
  if(!tab())return {};quint64 bytes{};int files{},dirs{},encrypted{};QSet<QString> methods;
  for(const auto& v:tab()->entries){auto e=v.toObject();bytes+=e["size"].toString().toULongLong();e["directory"].toBool()?++dirs:++files;if(e["encrypted"].toBool()){++encrypted;methods.insert(e["encryption"].toString("unknown"));}}
  QVariantList result;auto add=[&](QString label,QString value){result.append(QVariantMap{{"label",label},{"value",value}});};
  add(translate("size",QString::fromUtf8("大小")),EntryModel::bytes(bytes));add(translate("packed",QString::fromUtf8("压缩后")),EntryModel::bytes(tab()->physical));add(translate("files",QString::fromUtf8("文件")),QString::number(files));add(translate("folders",QString::fromUtf8("文件夹")),QString::number(dirs));add(translate("encryption",QString::fromUtf8("加密")),encrypted?QStringList(methods.values()).join(", "):"none");return result;
}
QString AppController::translate(QString key,QString fallback)const{return translations[key].toString(fallback);}
QVariantMap AppController::colors()const{for(const auto& value:skins){auto item=value.toObject();if(item["id"]==preferences["skin"]){QVariantMap result;const auto vars=item["vars"].toObject();for(auto it=vars.begin();it!=vars.end();++it)if(QColor(it.value().toString()).isValid())result[it.key()]=it.value().toString();return result;}}return {};}
QVariantList AppController::themeList()const{QVariantList result;for(const auto& v:skins){auto item=v.toObject();result.append(QVariantMap{{"id",item["id"].toString()},{"name",item["name"].toString()}});}return result;}
QVariantList AppController::languages()const{QVariantList result;for(const auto& name:QDir(":/data/i18n").entryList({"*.json"},QDir::Files)){auto code=QFileInfo(name).completeBaseName();if(code=="LOCALES")continue;QLocale locale(code);result.append(QVariantMap{{"code",code},{"name",locale.nativeLanguageName()+" ("+code+")"}});}return result;}
bool AppController::rtl()const{return QLocale(preferences["language"].toString()).textDirection()==Qt::RightToLeft;}
QString AppController::localPath(QUrl url)const{return url.isLocalFile()?QDir::cleanPath(url.toLocalFile()):QString();}
QJsonArray AppController::paths(QVariantList urls)const{QJsonArray result;for(const auto& v:urls){auto url=v.toUrl();auto path=localPath(url);if(path.isEmpty()&&v.metaType().id()==QMetaType::QString&&QFileInfo(v.toString()).isAbsolute())path=v.toString();if(!path.isEmpty())result.append(QFileInfo(path).absoluteFilePath());}return result;}
void AppController::openUrls(QVariantList urls){for(const auto& v:paths(urls)){
  QFileInfo info(v.toString());if(!info.isFile()){emit message(QString::fromUtf8("归档不存在：")+info.filePath(),true);continue;}
  const auto path=info.canonicalFilePath();QString id;for(const auto& t:documents)if(t.path==path){id=t.id;break;}
  if(id.isEmpty()){id=QUuid::createUuid().toString(QUuid::WithoutBraces);documents.append(Tab{id,path,{},{},{},{},0});}current=id;showTab();refresh(id);
}}
void AppController::showTab(){if(tab())model.load(tab()->entries,tab()->revision,tab()->folder);else model.load({},{});emit changed();}
void AppController::closeTab(QString id){for(int i=0;i<documents.size();++i)if(documents[i].id==id){documents.removeAt(i);break;}if(current==id)current=documents.isEmpty()?QString():documents.last().id;showTab();}
void AppController::activateTab(QString id){if(find(id)){current=id;showTab();}}
QString AppController::submit(QJsonObject request,Context context){auto id=jobs.submit(request);contexts[id]=std::move(context);return id;}
void AppController::refresh(QString id){if(auto t=find(id))submit({{"operation","list"},{"archive",t->path},{"codepage",codepage()}},{id,t->path});}
void AppController::testArchive(){if(tab())submit({{"operation","test"},{"archive",tab()->path}},{current,tab()->path});}
void AppController::addUrls(QVariantList urls){if(canModify())submit({{"operation","add"},{"archive",tab()->path},{"inputs",paths(urls)}},{current,tab()->path});}
void AppController::deleteSelected(){if(canModify()&&hasSelection())submit({{"operation","delete"},{"archive",tab()->path},{"entryIds",QJsonArray::fromStringList(model.selection())}},{current,tab()->path});}
void AppController::renameSelected(QString name){if(!canModify()||model.selection().size()!=1)return;const auto e=model.entry(model.selection().first());auto path=e["path"].toString();const auto slash=path.lastIndexOf('/');auto target=(slash>=0?path.left(slash+1):QString())+name;submit({{"operation","rename"},{"archive",tab()->path},{"entryId",model.selection().first()},{"newName",target}},{current,tab()->path});}
void AppController::createArchive(QVariantMap values){
  auto options=QJsonObject::fromVariantMap(values);auto name=values["name"].toString().trimmed();auto destination=values["destination"].toString();const auto format=values["format"].toString();
  if(name.isEmpty()||name.contains('/')||name.contains('\\')||name=="."||name==".."||!QFileInfo(destination).isAbsolute()){emit message(QString::fromUtf8("请指定有效的文件名和绝对目录"),true);return;}
  const auto suffix=format=="tgz"?"tar.gz":format;if(!name.endsWith('.'+suffix,Qt::CaseInsensitive))name+='.'+suffix;
  options["operation"]="create";options["output"]=QDir(destination).filePath(name);options["inputs"]=paths(values["inputs"].toList());if(options["volumeSize"].toString().trimmed().isEmpty())options.remove("volumeSize");
  submit(options,{{},options["output"].toString()});
}
void AppController::extractArchive(QVariantMap values){
  if(!tab())return;auto dest=values["destination"].toString();if(!QFileInfo(dest).isAbsolute()){emit message(QString::fromUtf8("请选择绝对目标目录"),true);return;}
  if(values["smart"].toBool()){QSet<QString> roots;for(const auto& v:tab()->entries){const auto e=v.toObject();const auto p=e["path"].toString();if(p.contains('/'))roots.insert(p.section('/',0,0));else if(!e["directory"].toBool())roots.insert(p);}if(roots.size()!=1){auto base=QFileInfo(tab()->path).completeBaseName();if(base.endsWith(".tar",Qt::CaseInsensitive))base.chop(4);dest=QDir(dest).filePath(base);}}
  submit({{"operation","extract"},{"archive",tab()->path},{"destination",dest},{"entryIds",QJsonArray::fromStringList(model.selection())},{"overwrite",values.value("overwrite","ask").toString()},{"codepage",codepage()}},{current,tab()->path,{},values["openAfter"].toBool()});
}
void AppController::activateEntry(QString id){if(model.enter(id)||!tab())return;const auto e=model.entry(id);if(e.isEmpty())return;
  auto temp=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/reazip-open-XXXXXX");if(!temp->isValid()){emit message(QString::fromUtf8("无法创建临时目录"),true);return;}
  const auto path=temp->path();temporary.push_back(std::move(temp));submit({{"operation","extract"},{"archive",tab()->path},{"destination",path},{"entryIds",QJsonArray{id}},{"overwrite","ask"},{"codepage",codepage()}},{current,tab()->path,e["path"].toString(),false,true});
}
void AppController::completed(QString id,QString op,QJsonObject payload){
  const auto context=contexts.take(id);
  if(op=="identity")identity=payload["identity"].toString();
  else if(op=="list"){if(auto t=find(context.tabId)){t->format=payload["format"].toString();t->revision=payload["revision"].toString();t->entries=payload["entries"].toArray();t->physical=payload["physicalSize"].toString().toULongLong();if(current==t->id)showTab();const auto action=launchActions.take(t->path);if(action=="extract_here")submit({{"operation","extract"},{"archive",t->path},{"destination",QFileInfo(t->path).absolutePath()},{"overwrite","ask"},{"codepage",codepage()}},{t->id,t->path});else if(action=="extract_to")emit extractRequested();}}
  else if(op=="create"){const auto outputs=payload["outputs"].toArray();if(!outputs.isEmpty())openUrls({QUrl::fromLocalFile(outputs.first().toString())});emit message(translate("success",QString::fromUtf8("压缩完成并已校验")),false);}
  else if(op=="extract"){
    const auto destination=payload["destination"].toString();if(context.drag){dragUrls.clear();for(const auto& name:QDir(destination).entryList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot))if(!name.startsWith(".reazip-"))dragUrls.append(QUrl::fromLocalFile(QDir(destination).filePath(name)));}else if(context.temporary){const auto file=QDir(destination).filePath(context.openEntry);if(executable(file))emit executableRequested(file);else openExternal(file);}else if(context.openAfter)openExternal(destination);
    emit message(translate("success",QString::fromUtf8("解压完成")),false);
  }else if(op=="test")emit message(translate("testOk",QString::fromUtf8("完整性测试通过")),false);
  else refresh(context.tabId);
  emit changed();emit operationCompleted(op,payload);
}
void AppController::failed(QString id,QString code,QString text){contexts.remove(id);emit message(code+": "+text,true);emit operationFailed(code,text);emit changed();}
void AppController::setPreference(QString key,QVariant value){preferences[key]=QJsonValue::fromVariant(value);if(key=="language"){translations=json(":/data/i18n/"+value.toString()+".json").object();}++epoch;emit changed();if(key=="codepage"&&tab())refresh(current);}
void AppController::openExternal(QString path){if(!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))emit message(QString::fromUtf8("无法打开：")+path,true);}
void AppController::openThemeDirectory(){const auto folder=platform::dataDirectory()+"/themes";if(QDir().mkpath(folder))openExternal(folder);else emit message(QString::fromUtf8("主题目录不可写：")+folder,true);}
void AppController::rescanThemes(){emit changed();}
void AppController::applyAssociations(){const auto error=platform::registerApplication(QCoreApplication::applicationFilePath());if(!error.isEmpty())emit message(error,true);else {
#ifdef Q_OS_WIN
  QDesktopServices::openUrl(QUrl("ms-settings:defaultapps"));
#endif
  emit message(QString::fromUtf8("已注册 REAzip，请在系统中选择默认应用"),false);}}
void AppController::handleLaunch(QJsonObject request){if(!platform::Instance::valid(request))return;QVariantList urls;for(const auto& v:request["paths"].toArray())urls.append(QUrl::fromLocalFile(v.toString()));const auto action=request["action"].toString();if(action=="add"){emit createRequested(urls);return;}for(const auto& v:request["paths"].toArray())if(action!="open")launchActions[QFileInfo(v.toString()).canonicalFilePath()]=action;openUrls(urls);}
void AppController::prepareDrag(){if(!tab()||!hasSelection()||busy())return;auto temp=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/reazip-drag-XXXXXX");if(!temp->isValid())return;const auto path=temp->path();temporary.push_back(std::move(temp));submit({{"operation","extract"},{"archive",tab()->path},{"destination",path},{"entryIds",QJsonArray::fromStringList(model.selection())},{"overwrite","ask"},{"codepage",codepage()}},{current,tab()->path,{},false,false,true});}
void AppController::beginPreparedDrag(){if(dragUrls.isEmpty())return;QDrag drag(this);auto mime=new QMimeData;mime->setUrls(dragUrls);drag.setMimeData(mime);drag.exec(Qt::CopyAction,Qt::CopyAction);}
