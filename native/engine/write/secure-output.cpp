#include "secure-output.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <random>
#include <streambuf>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>
#include <sddl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#endif
namespace reazip {
namespace {
[[noreturn]] void io(){
#ifdef _WIN32
  const auto code=GetLastError();
#else
  const auto code=errno;
#endif
  throw Error(ErrorCode::IoError,"Destination write or commit failed (OS "+std::to_string(code)+"); existing file retained");
}
[[noreturn]] void unsafe(){throw Error(ErrorCode::UnsafeEntry,"Destination contains a link or reparse point");}
std::string unique(){std::random_device random;return ".reazip-output-"+std::to_string(random())+"-"+std::to_string(random())+".tmp";}
#ifdef _WIN32
using Handle=HANDLE;
const Handle invalid=INVALID_HANDLE_VALUE;
void closeHandle(Handle h){if(h!=invalid)CloseHandle(h);}
struct Owned {Handle value=invalid;Owned()=default;explicit Owned(Handle h):value(h){}~Owned(){closeHandle(value);}Owned(Owned&& other)noexcept:value(other.value){other.value=invalid;}Owned(const Owned&)=delete;Owned& operator=(Owned&& other)noexcept{if(this!=&other){closeHandle(value);value=other.value;other.value=invalid;}return *this;}};
using CreateFn=NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
bool reparse(Handle h){FILE_ATTRIBUTE_TAG_INFO attributes{};if(!GetFileInformationByHandleEx(h,FileAttributeTagInfo,&attributes,sizeof(attributes)))io();return (attributes.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)!=0;}
Owned child(Handle parent,const fs::path& name,bool directory,bool create){
  static auto createFile=reinterpret_cast<CreateFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtCreateFile"));if(!createFile)io();
  const auto text=name.native();if(text.empty()||text.size()>32767)unsafe();
  UNICODE_STRING unicode{static_cast<USHORT>(text.size()*sizeof(wchar_t)),static_cast<USHORT>(text.size()*sizeof(wchar_t)),const_cast<wchar_t*>(text.data())};
  OBJECT_ATTRIBUTES object{};object.Length=sizeof(object);object.RootDirectory=parent;object.ObjectName=&unicode;object.Attributes=0x40;
  PSECURITY_DESCRIPTOR security{};if(create&&!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;FA;;;OW)(A;;FA;;;SY)",SDDL_REVISION_1,&security,nullptr))io();object.SecurityDescriptor=security;
  IO_STATUS_BLOCK status{};Handle result=invalid;
  const auto access=directory?(FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES|SYNCHRONIZE):(GENERIC_WRITE|DELETE|FILE_READ_ATTRIBUTES|SYNCHRONIZE);
  const auto options=0x00200000UL|0x00000020UL|(directory?0x00000001UL:0x00000040UL); // OPEN_REPARSE_POINT, synchronous, directory/non-directory
  const auto disposition=directory?(create?3UL:1UL):2UL; // OPEN_IF/OPEN/CREATE only
  const auto code=createFile(&result,access,&object,&status,nullptr,directory?FILE_ATTRIBUTE_DIRECTORY:FILE_ATTRIBUTE_NORMAL,directory?(FILE_SHARE_READ|FILE_SHARE_WRITE):(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),disposition,options,nullptr,0);
  if(security)LocalFree(security);
  if(code<0){if(static_cast<ULONG>(code)==0xC000050BUL||static_cast<ULONG>(code)==0xC0000279UL)unsafe();io();}
  Owned owned(result);if(reparse(result))unsafe();return owned;
}
class FileBuffer : public std::streambuf {
 public:Handle handle;explicit FileBuffer(Handle h):handle(h){}
 protected:std::streamsize xsputn(const char* p,std::streamsize n)override{std::streamsize count=0;while(count<n){DWORD written{};const auto amount=static_cast<DWORD>(std::min<std::streamsize>(n-count,0x7fffffff));if(!WriteFile(handle,p+count,amount,&written,nullptr)||!written)break;count+=written;}return count;}
 int overflow(int ch)override{if(ch==traits_type::eof())return traits_type::not_eof(ch);const char c=static_cast<char>(ch);return xsputn(&c,1)==1?ch:traits_type::eof();}
 pos_type seekoff(off_type offset,std::ios_base::seekdir direction,std::ios_base::openmode)override{LARGE_INTEGER distance{},position{};distance.QuadPart=offset;return SetFilePointerEx(handle,distance,&position,direction==std::ios_base::beg?FILE_BEGIN:direction==std::ios_base::cur?FILE_CURRENT:FILE_END)?pos_type(position.QuadPart):pos_type(off_type(-1));}
 pos_type seekpos(pos_type position,std::ios_base::openmode mode)override{return seekoff(off_type(position),std::ios_base::beg,mode);}
 int sync()override{return 0;}
};
#else
using Handle=int;constexpr Handle invalid=-1;void closeHandle(Handle h){if(h>=0)::close(h);}
struct Owned {Handle value=invalid;Owned()=default;explicit Owned(Handle h):value(h){}~Owned(){closeHandle(value);}Owned(Owned&& other)noexcept:value(other.value){other.value=invalid;}Owned(const Owned&)=delete;Owned& operator=(Owned&& other)noexcept{if(this!=&other){closeHandle(value);value=other.value;other.value=invalid;}return *this;}};
Owned child(Handle parent,const fs::path& name,bool directory,bool create){
  if(directory&&create&&mkdirat(parent,name.c_str(),0700)<0&&errno!=EEXIST)io();
  const int flags=directory?(O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC):(O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC);
  const auto fd=openat(parent,name.c_str(),flags,0600);if(fd<0){if(errno==ELOOP||errno==ENOTDIR)unsafe();io();}Owned result(fd);
  if(directory){struct stat attributes{};if(fstat(fd,&attributes)<0)io();if((attributes.st_uid!=geteuid()&&attributes.st_uid!=0)||((attributes.st_mode&(S_IWGRP|S_IWOTH))&&!(attributes.st_mode&S_ISVTX)))unsafe();}
  return result;
}
class FileBuffer : public std::streambuf {
 public:int handle;explicit FileBuffer(int h):handle(h){}
 protected:std::streamsize xsputn(const char* p,std::streamsize n)override{std::streamsize count=0;while(count<n){const auto size=::write(handle,p+count,static_cast<std::size_t>(n-count));if(size<0&&errno==EINTR)continue;if(size<=0)break;count+=size;}return count;}
 int overflow(int ch)override{if(ch==traits_type::eof())return traits_type::not_eof(ch);const char c=static_cast<char>(ch);return xsputn(&c,1)==1?ch:traits_type::eof();}
 pos_type seekoff(off_type offset,std::ios_base::seekdir direction,std::ios_base::openmode)override{const auto position=lseek(handle,offset,direction==std::ios_base::beg?SEEK_SET:direction==std::ios_base::cur?SEEK_CUR:SEEK_END);return position<0?pos_type(off_type(-1)):pos_type(position);}
 pos_type seekpos(pos_type position,std::ios_base::openmode mode)override{return seekoff(off_type(position),std::ios_base::beg,mode);}
 int sync()override{return 0;}
};
#endif
}
struct SecureOutput::Impl {
  fs::path path;std::vector<Owned> roots;
  explicit Impl(fs::path value):path(fs::absolute(value).lexically_normal()){
    auto components=path.relative_path();
#ifdef _WIN32
    auto text=path.native();if(text.compare(0,8,L"\\\\?\\UNC\\")==0)text=L"\\\\"+text.substr(8);else if(text.compare(0,4,L"\\\\?\\")==0)text=text.substr(4);std::wstring nativeBase;
    if(text.size()>=3&&text[1]==L':'){nativeBase=L"\\\\?\\"+text.substr(0,3);components=fs::path(text.substr(3));}
    else if(text.compare(0,2,L"\\\\")==0){const auto server=text.find(L'\\',2);if(server==std::wstring::npos)unsafe();const auto share=text.find(L'\\',server+1);const auto base=share==std::wstring::npos?text:text.substr(0,share+1);nativeBase=L"\\\\?\\UNC\\"+base.substr(2);components=share==std::wstring::npos?fs::path():fs::path(text.substr(share+1));}else unsafe();
    auto root=CreateFileW(nativeBase.c_str(),FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(root==invalid)io();roots.emplace_back(root);if(reparse(root))unsafe();
#else
    auto root=::open("/",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(root<0)io();roots.emplace_back(root);
#endif
    for(const auto& component:components){if(component=="."||component=="..")unsafe();roots.push_back(child(roots.back().value,component,true,true));}
  }
  std::vector<Owned> parents(const fs::path& relative,bool create=true){std::vector<Owned> handles;auto parent=roots.back().value;for(const auto& component:relative){if(component.empty()||component=="."||component==".."||component.has_root_path())unsafe();handles.push_back(child(parent,component,true,create));parent=handles.back().value;}return handles;}
};
SecureOutput::SecureOutput(const fs::path& root):impl(std::make_unique<Impl>(root)){}
SecureOutput::~SecureOutput()=default;
void SecureOutput::directory(const fs::path& relative){impl->parents(relative);}
void SecureOutput::file(const fs::path& relative,bool replace,const std::function<void(std::ostream&)>& writer,const Callbacks& cb){
  auto handles=impl->parents(relative.parent_path());const auto parent=handles.empty()?impl->roots.back().value:handles.back().value;
  const auto temporary=fs::u8path(unique());auto file=child(parent,temporary,false,true);bool committed=false;
  auto cleanup=[&]{if(!committed){
#ifdef _WIN32
    FILE_DISPOSITION_INFO remove{TRUE};SetFileInformationByHandle(file.value,FileDispositionInfo,&remove,sizeof(remove));
#else
    unlinkat(parent,temporary.c_str(),0);
#endif
  }};
  try{
    FileBuffer buffer(file.value);std::ostream stream(&buffer);
    try{writer(stream);}catch(...){if(!stream)io();throw;}
    if(!stream)io();
#ifdef _WIN32
    if(!FlushFileBuffers(file.value))io();
#else
    if(fsync(file.value)<0)io();
#endif
    if(cb.cancelled&&cb.cancelled())throw Error(ErrorCode::Cancelled,"Task cancelled");
    if(cb.outputFileBeforeCommit)cb.outputFileBeforeCommit(impl->path/relative.parent_path()/temporary);
    auto current=impl->parents(relative.parent_path(),false);const auto now=current.empty()?impl->roots.back().value:current.back().value;
#ifdef _WIN32
    if(reparse(parent)||reparse(now))unsafe();
#else
    struct stat before{},after{};if(fstat(parent,&before)<0||fstat(now,&after)<0)io();if(before.st_dev!=after.st_dev||before.st_ino!=after.st_ino)unsafe();
#endif
    if(cb.commitState)cb.commitState(true);
    try{
#ifdef _WIN32
      const auto name=relative.filename().native();std::vector<unsigned char> storage(sizeof(FILE_RENAME_INFO)+name.size()*sizeof(wchar_t));auto rename=reinterpret_cast<FILE_RENAME_INFO*>(storage.data());std::memset(rename,0,storage.size());rename->ReplaceIfExists=replace;rename->RootDirectory=parent;rename->FileNameLength=static_cast<DWORD>(name.size()*sizeof(wchar_t));std::memcpy(rename->FileName,name.data(),rename->FileNameLength);
      using SetFn=NTSTATUS(NTAPI*)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);static auto setFile=reinterpret_cast<SetFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtSetInformationFile"));if(!setFile)io();IO_STATUS_BLOCK status{};const auto result=setFile(file.value,&status,rename,static_cast<ULONG>(storage.size()),static_cast<FILE_INFORMATION_CLASS>(10));if(result<0){SetLastError(static_cast<DWORD>(result));io();}
#else
      if(replace){if(renameat(parent,temporary.c_str(),parent,relative.filename().c_str())<0)io();}
      else {
#ifdef __linux__
        if(renameat2(parent,temporary.c_str(),parent,relative.filename().c_str(),RENAME_NOREPLACE)<0)io();
#elif defined(__APPLE__)
        if(renameatx_np(parent,temporary.c_str(),parent,relative.filename().c_str(),RENAME_EXCL)<0)io();
#else
        if(linkat(parent,temporary.c_str(),parent,relative.filename().c_str(),0)<0)io();if(unlinkat(parent,temporary.c_str(),0)<0)io();
#endif
      }
      if(fsync(parent)<0)io();
#endif
      committed=true;
    }catch(...){if(cb.commitState)cb.commitState(false);throw;}
    if(cb.commitState)cb.commitState(false);
  }catch(...){cleanup();throw;}
}
}
