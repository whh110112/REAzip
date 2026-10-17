#include <reazip/engine.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>
#include "../platform/Platform.hpp"
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
using namespace reazip;
namespace {
constexpr std::size_t maximumFrame=16*1024*1024;
std::string text(const QJsonValue& v){if(!v.isString())throw Error(ErrorCode::UnsupportedOperation,"Expected a string field");return v.toString().toUtf8().toStdString();}
fs::path file(const QJsonValue& v){return fs::u8path(text(v));}
QString qs(const std::string& v){return QString::fromUtf8(v.data(),static_cast<qsizetype>(v.size()));}
std::vector<std::string> strings(const QJsonValue& v){if(v.isUndefined())return {};if(!v.isArray())throw Error(ErrorCode::UnsupportedOperation,"Expected array");std::vector<std::string> result;for(const auto& x:v.toArray())result.push_back(text(x));return result;}
QString errorName(ErrorCode code){switch(code){
#define CASE(x) case ErrorCode::x:return #x;
CASE(PasswordRequired) CASE(WrongPassword) CASE(UnsupportedFormat) CASE(UnsupportedOperation) CASE(EngineMissing) CASE(EngineArchitectureMismatch) CASE(PermissionDenied) CASE(CorruptArchive) CASE(UnsafeEntry) CASE(Cancelled) CASE(IoError)
#undef CASE
}return "IoError";}
Overwrite policy(const QString& s){if(s=="overwrite")return Overwrite::Overwrite;if(s=="skip")return Overwrite::Skip;if(s=="rename")return Overwrite::Rename;return Overwrite::Ask;}
class Session {
 public:
  std::atomic<bool> stop{false},done{false};std::string id;
  std::mutex output,challengeMutex;std::condition_variable challengeChanged;
  QString challengeId;QJsonObject answer;bool answered{};std::uint64_t sequence{};
  bool terminalSent{};
  void send(QString type,QJsonObject body={}){
    std::lock_guard<std::mutex> lock(output);
    const bool terminal=type=="completed"||type=="failed"||type=="cancelled";
    if(terminalSent)return;if(terminal){terminalSent=true;done=true;}
    body["protocolVersion"]=1;body["jobId"]=qs(id);body["type"]=type;body["sequence"]=QString::number(++sequence);
    const auto data=QJsonDocument(body).toJson(QJsonDocument::Compact);std::cout.write(data.constData(),data.size());std::cout<<'\n'<<std::flush;
  }
  QJsonObject ask(QString type,QJsonObject details={}) {
    std::unique_lock<std::mutex> lock(challengeMutex);challengeId=QUuid::createUuid().toString(QUuid::WithoutBraces);answered=false;answer={};
    details["challengeId"]=challengeId;send(type,details);challengeChanged.wait(lock,[&]{return answered||stop.load();});
    if(stop)throw Error(ErrorCode::Cancelled,"Task cancelled");auto result=answer;answer={};challengeId.clear();return result;
  }
  void respond(const QJsonObject& message){std::lock_guard<std::mutex> lock(challengeMutex);if(!challengeId.isEmpty()&&message["challengeId"].toString()==challengeId&&!answered){answer=message;answered=true;challengeChanged.notify_all();}}
  void cancel(){stop=true;challengeChanged.notify_all();}
  void execute(QJsonObject request,const fs::path& library) {
    send("started");
    try{
      Engine engine(library);Callbacks cb;cb.cancelled=[&]{return stop.load();};
      cb.progress=[&](auto current,auto total){send("progress",{{"current",QString::number(current)},{"total",QString::number(total)}});};
      cb.commitState=[&](bool value){send("progress",{{"phase",value?"committing":"running"}});};
      cb.overwrite=[&](const fs::path& p){auto a=ask("overwriteRequired",{{"path",qs(p.u8string())}});if(a["choice"].toString()=="cancel"){cancel();throw Error(ErrorCode::Cancelled,"Task cancelled");}return policy(a["choice"].toString());};
      cb.trash=[](const fs::path& p){return QFile::moveToTrash(qs(p.u8string()));};
      cb.outputFileBeforeCommit=[&](const fs::path& p){QString error;if(!platform::propagateOrigin(request["archive"].toString(),qs(p.u8string()),&error))throw Error(ErrorCode::IoError,error.toStdString());};
      const auto operation=text(request["operation"]);std::string password=request["password"].toString().toUtf8().toStdString();
      std::unique_ptr<QLockFile> archiveLock;
      if(operation!="identity"){
        QFileInfo info(request[operation=="create"?"output":"archive"].toString());
        auto canonical=info.canonicalFilePath();if(canonical.isEmpty())canonical=QDir(info.absolutePath()).canonicalPath()+"/"+info.fileName();
#ifdef _WIN32
        canonical=canonical.toLower();
#endif
        const auto folder=QStandardPaths::writableLocation(QStandardPaths::TempLocation)+"/reazip-native-locks";
        if(!QDir().mkpath(folder))throw Error(ErrorCode::PermissionDenied,"Could not create task lock directory");
        const auto hash=QCryptographicHash::hash(canonical.toUtf8(),QCryptographicHash::Sha256).toHex();
        archiveLock=std::make_unique<QLockFile>(folder+"/"+QString::fromLatin1(hash)+".lock");
        while(!archiveLock->tryLock(100))if(stop)throw Error(ErrorCode::Cancelled,"Task cancelled while waiting for archive lock");
      }
      for(int attempt=0;;++attempt){
        try{
          if(stop)throw Error(ErrorCode::Cancelled,"Task cancelled");
          QJsonObject result;
          if(operation=="identity")result["identity"]=qs(engine.identity());
          else if(operation=="list"){
            const auto listing=engine.list(file(request["archive"]),password,request["codepage"].toString().toStdString(),cb);
            result={{"format",qs(listing.format)},{"revision",qs(listing.revision)},{"physicalSize",QString::number(listing.physicalSize)}};
            QJsonArray chunk;
            for(const auto& e:listing.entries){QJsonObject entry{{"entryId",qs(e.id)},{"index",QString::number(e.index)},{"path",qs(e.path)},{"directory",e.directory},{"link",e.link},{"size",QString::number(e.size)},{"encrypted",e.encrypted},{"encryption",qs(e.encryption)}};if(e.packed)entry["packedSize"]=QString::number(*e.packed);if(e.modified)entry["modified"]=QDateTime::fromSecsSinceEpoch(*e.modified).toLocalTime().toString("yyyy-MM-dd HH:mm");chunk.append(entry);if(chunk.size()==512){send("entriesChunk",{{"entries",chunk}});chunk={};}}
            if(!chunk.isEmpty())send("entriesChunk",{{"entries",chunk}});
          }else if(operation=="test")engine.test(file(request["archive"]),password,cb);
          else if(operation=="extract"){
            ExtractOptions opts;opts.destination=file(request["destination"]);opts.entryIds=strings(request["entryIds"]);opts.password=password;opts.codepage=request["codepage"].toString().toStdString();opts.overwrite=policy(request["overwrite"].toString());engine.extract(file(request["archive"]),opts,cb);
            result["destination"]=qs(opts.destination.u8string());
          }else if(operation=="create"){
            CreateOptions opts;opts.output=file(request["output"]);for(const auto& p:strings(request["inputs"]))opts.inputs.push_back(fs::u8path(p));opts.format=text(request["format"]);opts.password=password;opts.level=request["level"].toInt(5);opts.encryptHeaders=request["encryptHeaders"].toBool();opts.deleteSources=request["deleteSources"].toBool();opts.aes=request["aes"].toBool(true);
            if(request.contains("volumeSize")){bool ok=false;opts.volumeSize=request["volumeSize"].toString().toULongLong(&ok);if(!ok)throw Error(ErrorCode::UnsupportedOperation,"Invalid volume size");}
            QJsonArray outputs;for(const auto& p:engine.create(opts,cb))outputs.append(qs(p.u8string()));result["outputs"]=outputs;
          }else if(operation=="delete")engine.remove(file(request["archive"]),strings(request["entryIds"]),password,cb);
          else if(operation=="rename")engine.rename(file(request["archive"]),text(request["entryId"]),text(request["newName"]),password,cb);
          else if(operation=="add") {std::vector<fs::path> paths;for(const auto& p:strings(request["inputs"]))paths.push_back(fs::u8path(p));engine.add(file(request["archive"]),paths,password,cb);}
          else throw Error(ErrorCode::UnsupportedOperation,"Unknown operation");
          password.clear();request.remove("password");send("completed",result);break;
        }catch(const Error& e){
          if((e.code==ErrorCode::PasswordRequired||e.code==ErrorCode::WrongPassword)&&attempt<10){auto answer=ask("passwordRequired",{{"incorrect",e.code==ErrorCode::WrongPassword}});password=text(answer["password"]);continue;}throw;
        }
      }
    }catch(const Error& e){send(e.code==ErrorCode::Cancelled?"cancelled":"failed",{{"code",errorName(e.code)},{"message",QString::fromUtf8(e.what())}});}
    catch(const std::exception&){send("failed",{{"code","IoError"},{"message","File or native engine operation failed"}});}
    done=true;
  }
};
}
int main(int argc,char** argv){
  QCoreApplication app(argc,argv);const auto args=app.arguments();const int option=args.indexOf("--engine");
  if(option<0||option+1>=args.size())return 2;
  const auto library=fs::u8path(args[option+1].toUtf8().toStdString());Session session;std::thread job;session.send("ready");
  std::string line;char c;bool submitted=false;
  while(std::cin.get(c)){
    if(c!='\n'){line.push_back(c);if(line.size()>maximumFrame){session.send("failed",{{"code","ProtocolError"}});session.cancel();break;}continue;}
    QJsonParseError parse;const auto document=QJsonDocument::fromJson(QByteArray::fromStdString(line),&parse);line.clear();
    if(parse.error!=QJsonParseError::NoError||!document.isObject()){session.send("failed",{{"code","ProtocolError"}});session.cancel();break;}
    const auto message=document.object();const auto type=message["type"].toString();
    if(message["protocolVersion"].toInt()!=1){session.send("failed",{{"code","ProtocolVersionMismatch"}});session.cancel();break;}
    if(type=="hello"){session.send("ready");continue;}
    if(type=="submit"&&!submitted&&!message["jobId"].toString().isEmpty()){
      session.id=message["jobId"].toString().toStdString();submitted=true;job=std::thread([&,message]{session.execute(message,library);});
    }else if(submitted&&message["jobId"].toString().toStdString()==session.id&&!session.done){
      if(type=="cancel")session.cancel();else if(type=="respondChallenge")session.respond(message);
    }else {session.send("failed",{{"code","ProtocolStateError"}});session.cancel();break;}
  }
  if(!line.empty()&&!session.done)session.cancel();if(!session.done)session.cancel();if(job.joinable())job.join();return 0;
}
