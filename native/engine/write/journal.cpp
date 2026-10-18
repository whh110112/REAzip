#include "journal.hpp"
#include "../read/temporary.hpp"
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <aclapi.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#endif
namespace zenzip {
namespace {
[[noreturn]] void failure(const fs::path& stage={}){throw Error(ErrorCode::IoError,stage.empty()?"Archive I/O or recovery failed":"Recovery data retained; inspect this directory: "+stage.u8string());}
std::uint64_t digest(const fs::path& p){std::ifstream file(p,std::ios::binary);if(!file)failure();std::uint64_t hash=14695981039346656037ull;char buffer[65536];while(file){file.read(buffer,sizeof(buffer));for(std::streamsize i=0;i<file.gcount();++i){hash^=static_cast<unsigned char>(buffer[i]);hash*=1099511628211ull;}}if(!file.eof())failure();return hash;}
void syncDirectory(const fs::path& path){
#ifndef _WIN32
  const auto fd=open(path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)failure();const auto result=fsync(fd);close(fd);if(result<0)failure();
#else
  (void)path; // file flushes plus MoveFileEx WRITE_THROUGH are used on Windows.
#endif
}
class Lease {
 public:
#ifdef _WIN32
  HANDLE handle=INVALID_HANDLE_VALUE;
  explicit Lease(const fs::path& p,bool create){handle=CreateFileW(p.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,create?CREATE_NEW:OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);}
  bool valid()const{return handle!=INVALID_HANDLE_VALUE;}
  ~Lease(){if(valid())CloseHandle(handle);}
#else
  int handle=-1;
  explicit Lease(const fs::path& p,bool create){handle=open(p.c_str(),O_RDWR|O_NOFOLLOW|O_CLOEXEC|(create?(O_CREAT|O_EXCL):0),0600);if(handle>=0&&flock(handle,LOCK_EX|LOCK_NB)<0){close(handle);handle=-1;}}
  bool valid()const{return handle>=0;}
  ~Lease(){if(valid())close(handle);}
#endif
};
bool owner(const fs::path& p){
#ifdef _WIN32
  const auto attributes=GetFileAttributesW(p.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||!(attributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
  PSID sid{};PSECURITY_DESCRIPTOR descriptor{};if(GetNamedSecurityInfoW(const_cast<wchar_t*>(p.c_str()),SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&sid,nullptr,nullptr,nullptr,&descriptor)!=ERROR_SUCCESS)return false;
  HANDLE token{};bool same=false;if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)){DWORD size{};GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> info(size);if(GetTokenInformation(token,TokenUser,info.data(),size,&size))same=EqualSid(sid,reinterpret_cast<TOKEN_USER*>(info.data())->User.Sid);CloseHandle(token);}LocalFree(descriptor);return same;
#else
  struct stat value{};return lstat(p.c_str(),&value)==0&&S_ISDIR(value.st_mode)&&value.st_uid==geteuid()&&!(value.st_mode&(S_IWGRP|S_IWOTH));
#endif
}
struct Item {std::string file,target,backup;bool existed{},replace{};std::uint64_t oldHash{},newHash{};};
struct Record {fs::path stage,parent;std::string primary;bool complete{};std::vector<Item> items;};
void number(std::ostream& stream,std::uint64_t value){for(int i=0;i<8;++i){stream.put(static_cast<char>(value&255));value>>=8;}}
std::uint64_t number(std::istream& stream){std::uint64_t value=0;for(int i=0;i<8;++i){const auto byte=stream.get();if(byte<0)failure();value|=static_cast<std::uint64_t>(byte)<<(i*8);}return value;}
void string(std::ostream& stream,const std::string& value){number(stream,value.size());stream.write(value.data(),value.size());}
std::string string(std::istream& stream){const auto size=number(stream);if(size>65536)failure();std::string value(size,'\0');stream.read(value.data(),size);if(!stream)failure();return value;}
bool leaf(const std::string& value){auto p=fs::u8path(value);return !value.empty()&&value!="."&&value!=".."&&p==p.filename()&&value.find('\0')==std::string::npos&&value.find('/')==std::string::npos&&value.find('\\')==std::string::npos&&value.find(':')==std::string::npos;}
void save(const Record& record){const auto path=record.stage/"journal.next";{std::ofstream stream(path,std::ios::binary|std::ios::trunc);stream.write("ZENZIPJ1",8);number(stream,record.complete);string(stream,record.primary);number(stream,record.items.size());for(const auto& item:record.items){string(stream,item.file);string(stream,item.target);string(stream,item.backup);number(stream,item.existed);number(stream,item.replace);number(stream,item.oldHash);number(stream,item.newHash);}stream.close();if(!stream)failure();}syncFile(path);commitFile(path,record.stage/"journal.bin");syncDirectory(record.stage);}
Record read(const fs::path& stage){Record record;record.stage=stage;record.parent=stage.parent_path();std::ifstream stream(stage/"journal.bin",std::ios::binary);char magic[8];stream.read(magic,8);if(!stream||(std::string(magic,8)!="ZENZIPJ1"&&std::string(magic,8)!="REAZIPJ1"))failure();record.complete=number(stream)!=0;record.primary=string(stream);if(!leaf(record.primary))failure();const auto count=number(stream);if(!count||count>100000)failure();for(std::uint64_t i=0;i<count;++i){Item item;item.file=string(stream);item.target=string(stream);item.backup=string(stream);item.existed=number(stream)!=0;item.replace=number(stream)!=0;item.oldHash=number(stream);item.newHash=number(stream);if(!leaf(item.file)||!leaf(item.target)||!leaf(item.backup))failure();record.items.push_back(item);}return record;}
bool rollbackRecord(const Record& record){try{
  // Validate the complete set first; never overwrite a file changed after the interrupted job.
  for(const auto& item:record.items){const auto target=record.parent/fs::u8path(item.target);if(fs::exists(target)){if(fs::is_symlink(fs::symlink_status(target)))failure();const auto actual=digest(target);if(actual!=item.newHash&&(!item.existed||actual!=item.oldHash))failure();}if(item.existed&&digest(record.stage/fs::u8path(item.backup))!=item.oldHash)failure();}
  for(std::size_t i=0;i<record.items.size();++i){const auto& item=record.items[i];const auto target=record.parent/fs::u8path(item.target);if(item.existed){if(fs::exists(target)&&digest(target)==item.oldHash)continue;const auto copy=record.stage/("restore-"+std::to_string(i));fs::copy_file(record.stage/fs::u8path(item.backup),copy,fs::copy_options::overwrite_existing);syncFile(copy);commitFile(copy,target);}else if(fs::exists(target))fs::remove(target);}
  syncDirectory(record.parent);return true;
}catch(...){return false;}}
}
void syncFile(const fs::path& p){
#ifdef _WIN32
  auto file=CreateFileW(p.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file==INVALID_HANDLE_VALUE)failure();const auto ok=FlushFileBuffers(file);CloseHandle(file);if(!ok)failure();
#else
  const auto file=open(p.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(file<0)failure();const auto result=fsync(file);close(file);if(result<0)failure();
#endif
}
void commitFile(const fs::path& source,const fs::path& target,bool replace){
#ifdef _WIN32
  if(!MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH|(replace?MOVEFILE_REPLACE_EXISTING:0)))failure();
#else
  if(replace)fs::rename(source,target);else {
#ifdef __linux__
    if(renameat2(AT_FDCWD,source.c_str(),AT_FDCWD,target.c_str(),RENAME_NOREPLACE)<0)failure();
#elif defined(__APPLE__)
    if(renamex_np(source.c_str(),target.c_str(),RENAME_EXCL)<0)failure();
#else
    if(::link(source.c_str(),target.c_str())<0)failure();fs::remove(source);
#endif
  }syncDirectory(target.parent_path());
#endif
}
struct OutputJournal::Impl {Record record;std::unique_ptr<Lease> lease;bool committed{};};
OutputJournal::OutputJournal(fs::path stage,fs::path primary,std::vector<fs::path> files,std::vector<fs::path> outputs,std::vector<bool> replace):impl(std::make_unique<Impl>()){
  auto& record=impl->record;record.stage=std::move(stage);record.parent=record.stage.parent_path();record.primary=primary.filename().u8string();impl->lease=std::make_unique<Lease>(record.stage/"journal.lock",true);if(!impl->lease->valid()||files.size()!=outputs.size()||files.size()!=replace.size()||!leaf(record.primary))failure();
  for(std::size_t i=0;i<files.size();++i){if(files[i].parent_path()!=record.stage||outputs[i].parent_path()!=record.parent)failure();Item item;item.file=files[i].filename().u8string();item.target=outputs[i].filename().u8string();item.backup="backup-"+std::to_string(i);item.replace=replace[i];item.existed=fs::exists(outputs[i]);if(item.existed&&!item.replace)failure();syncFile(files[i]);item.newHash=digest(files[i]);if(item.existed){const auto backup=record.stage/fs::u8path(item.backup);fs::copy_file(outputs[i],backup);syncFile(backup);item.oldHash=digest(backup);}record.items.push_back(std::move(item));}
  save(record);
}
OutputJournal::~OutputJournal()=default;
void OutputJournal::commit(const Callbacks& cb){
  for(std::size_t i=0;i<impl->record.items.size();++i){if(cb.cancelled&&cb.cancelled())throw Error(ErrorCode::Cancelled,"Task cancelled before commit completed");const auto& item=impl->record.items[i];commitFile(impl->record.stage/fs::u8path(item.file),impl->record.parent/fs::u8path(item.target),item.replace);if(cb.outputCommitted)cb.outputCommitted(i);}
  impl->record.complete=true;save(impl->record);impl->committed=true;
}
bool OutputJournal::rollback(){return impl->committed||rollbackRecord(impl->record);}
void recoverTransactions(const fs::path& archive,const Callbacks& cb){
  const auto parent=fs::weakly_canonical(fs::absolute(archive).parent_path());if(!fs::is_directory(parent))return;
  for(const auto& entry:fs::directory_iterator(parent)){if((entry.path().filename().u8string().find(".zenzip-txn-")!=0&&entry.path().filename().u8string().find(".reazip-txn-")!=0)||!owner(entry.path())||!fs::is_regular_file(entry.path()/"journal.bin"))continue;
    bool cleanup=false;{Lease lease(entry.path()/"journal.lock",false);if(!lease.valid())continue;const auto record=read(entry.path());const auto name=archive.filename().u8string();if(record.primary!=name&&std::none_of(record.items.begin(),record.items.end(),[&](const Item& item){return item.target==name;}))continue;requirePrivateAncestors(parent);if(cb.cancelled&&cb.cancelled())throw Error(ErrorCode::Cancelled,"Recovery cancelled");if(!record.complete&&!rollbackRecord(record))failure(entry.path());cleanup=true;}
    if(cleanup)fs::remove_all(entry.path());
  }
}
}
