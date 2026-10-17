#include <reazip/engine.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
namespace fs=reazip::fs;
void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::string bytes(fs::path p){
#ifdef _WIN32
  auto native=fs::absolute(p).make_preferred().native();if(native.compare(0,4,L"\\\\?\\")!=0)p=fs::path(L"\\\\?\\"+native);
#endif
  std::ifstream s(p,std::ios::binary);return {(std::istreambuf_iterator<char>(s)),{}};}
int main(int argc,char** argv){try{
  require(argc==4,"expected library/work/fixtures");reazip::Engine engine(fs::absolute(fs::u8path(argv[1])));
  const auto fixtures=fs::absolute(fs::u8path(argv[3]));
  const auto work=fs::absolute(fs::u8path(argv[2]))/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());fs::create_directories(work);const auto source=work/fs::u8path(u8"原始 😀.txt");std::ofstream(source)<<"original contents";
  reazip::Callbacks cb;cb.overwrite=[](const auto&){return reazip::Overwrite::Overwrite;};
  for(const auto& format:{"zip","7z","tar","tgz"}){
    reazip::CreateOptions options;options.output=work/(std::string("output.")+format);options.inputs={source};options.format=format;
    engine.create(options,cb);const auto listing=engine.list(options.output);
    reazip::ExtractOptions extract;extract.destination=work/(std::string("out-")+format);extract.overwrite=reazip::Overwrite::Overwrite;
    engine.extract(options.output,extract);require(bytes(extract.destination/source.filename())=="original contents","native archive roundtrip");
    if(std::string(format)=="zip"){
      auto target=extract.destination/source.filename();std::ofstream(target)<<"keep";
      extract.entryIds={listing.entries.front().id};extract.overwrite=reazip::Overwrite::Ask;
      int asked=0;reazip::Callbacks ask;ask.overwrite=[&](const auto&){++asked;return reazip::Overwrite::Skip;};engine.extract(options.output,extract,ask);
      require(asked==1&&bytes(target)=="keep","selected extract must ask and skip");
      ask.overwrite=[](const auto&){return reazip::Overwrite::Rename;};engine.extract(options.output,extract,ask);require(bytes(target)=="keep","rename preserves original");
      engine.rename(options.output,listing.entries.front().id,"renamed.txt",{},cb);auto updated=engine.list(options.output);require(updated.entries.front().path=="renamed.txt","rename entry");
      const auto added=work/"added.txt";std::ofstream(added)<<"added";engine.add(options.output,{added},{},cb);updated=engine.list(options.output);require(updated.entries.size()==2,"add entry");
      engine.remove(options.output,{updated.entries.front().id},{},cb);require(engine.list(options.output).entries.size()==1,"delete selected entry");
    }
  }
  reazip::CreateOptions encrypted;encrypted.output=work/"protected.zip";encrypted.inputs={source};encrypted.password="test-only";engine.create(encrypted,cb);
  require(engine.list(encrypted.output).entries[0].encryption=="AES","ZIP create must explicitly use AES");engine.test(encrypted.output,"test-only");
  auto before=bytes(encrypted.output);reazip::Callbacks cancel;cancel.cancelled=[](){return true;};
  bool cancelled=false;try{engine.create(encrypted,cancel);}catch(const reazip::Error& e){cancelled=e.code==reazip::ErrorCode::Cancelled;}
  require(cancelled&&bytes(encrypted.output)==before&&fs::exists(source),"cancel preserves archive and source");
  for(const auto& fixture:{"escape.zip","symlink.zip"}){
    bool blocked=false;reazip::ExtractOptions bad;bad.destination=work/fixture;
    try{engine.extract(fixtures/fixture,bad);}catch(const reazip::Error& e){blocked=e.code==reazip::ErrorCode::UnsafeEntry;}
    require(blocked&&!fs::exists(work/"escaped.txt"),"unsafe paths and links must be rejected");
  }
  encrypted.output=work/"headers.7z";encrypted.format="7z";encrypted.encryptHeaders=true;engine.create(encrypted,cb);
  auto protectedList=engine.list(encrypted.output,"test-only");engine.rename(encrypted.output,protectedList.entries.front().id,"changed.txt","test-only",cb);
  bool stillProtected=false;try{engine.list(encrypted.output);}catch(const reazip::Error& e){stillProtected=e.code==reazip::ErrorCode::PasswordRequired;}
  require(stillProtected,"editing must retain header encryption");
  const auto bulk=work/"bulk.bin";{std::ofstream s(bulk,std::ios::binary);std::string block(65536,'a');for(int i=0;i<128;++i)s<<block;}
  encrypted.inputs={bulk};encrypted.output=work/"cancelled.7z";encrypted.deleteSources=true;
  bool stopping=false,trashCalled=false;reazip::Callbacks mid;
  mid.cancelled=[&](){return stopping;};mid.progress=[&](auto,auto){stopping=true;};mid.trash=[&](const auto&){trashCalled=true;return false;};
  cancelled=false;try{engine.create(encrypted,mid);}catch(const reazip::Error& e){cancelled=e.code==reazip::ErrorCode::Cancelled;}
  require(cancelled&&!trashCalled&&fs::exists(bulk)&&!fs::exists(encrypted.output),"mid-operation cancellation must preserve sources");
  reazip::CreateOptions split;split.output=work/"volumes.7z";split.inputs={bulk};split.format="7z";split.level=0;split.volumeSize=1024*1024;
  const auto volumes=engine.create(split,cb);require(volumes.size()>1,"split archive outputs");engine.test(volumes.front());
  auto longRoot=work;for(int i=0;i<10;++i)longRoot/="long-route-abcdefghijklmnopqrstuvwxyz";reazip::CreateOptions longOptions;longOptions.inputs={source};longOptions.output=longRoot/"archive.zip";engine.create(longOptions,cb);require(engine.list(longOptions.output).entries.size()==1,"long path archive list");reazip::ExtractOptions longExtract;longExtract.destination=longRoot/"extracted";longExtract.overwrite=reazip::Overwrite::Overwrite;engine.extract(longOptions.output,longExtract);require(bytes(longExtract.destination/source.filename())=="original contents","long path extraction content");longOptions.inputs={longExtract.destination/source.filename()};longOptions.output=work/"from-long-input.zip";engine.create(longOptions,cb);engine.test(longOptions.output);
  const auto deletion=work/"trash-source.txt",recycle=work/"recovered-source.txt";std::ofstream(deletion)<<"recoverable";reazip::CreateOptions del;del.inputs={deletion};del.output=work/"trash-verified.zip";del.deleteSources=true;bool trashed=false;reazip::Callbacks trash;trash.trash=[&](const auto& path){engine.test(del.output);fs::rename(path,recycle);trashed=true;return true;};engine.create(del,trash);require(trashed&&!fs::exists(deletion)&&bytes(recycle)=="recoverable","source trash must happen after archive verification");
  std::cout<<"Native write regressions PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
