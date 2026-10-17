#pragma once
#include <reazip/types.hpp>
#include <random>
#include <cerrno>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace reazip {
inline bool privateDirectory(const fs::path& path) {
#ifdef _WIN32
  HANDLE token{};if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))throw Error(ErrorCode::PermissionDenied,"Cannot query private directory owner");DWORD size{};GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> info(size);const bool ok=GetTokenInformation(token,TokenUser,info.data(),size,&size);CloseHandle(token);if(!ok)throw Error(ErrorCode::PermissionDenied,"Cannot query private directory owner");LPWSTR sid{};if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(info.data())->User.Sid,&sid))throw Error(ErrorCode::PermissionDenied,"Cannot encode private directory owner");
  const auto descriptor=std::wstring(L"O:")+sid+L"D:P(A;;FA;;;"+sid+L")(A;;FA;;;SY)";LocalFree(sid);PSECURITY_DESCRIPTOR security{};if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(),SDDL_REVISION_1,&security,nullptr))throw Error(ErrorCode::PermissionDenied,"Cannot protect private directory");SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),security,FALSE};const bool made=CreateDirectoryW(path.c_str(),&attributes);const auto error=GetLastError();LocalFree(security);if(!made&&error!=ERROR_ALREADY_EXISTS)throw Error(ErrorCode::PermissionDenied,"Cannot create private directory");return made;
#else
  if(::mkdir(path.c_str(),0700)==0){::chmod(path.c_str(),0700);return true;}if(errno==EEXIST)return false;throw Error(ErrorCode::PermissionDenied,"Cannot create private directory");
#endif
}
inline void requirePrivateAncestors(const fs::path& parent){
#ifndef _WIN32
  auto path=parent.root_path();for(const auto& part:parent.relative_path()){path/=part;struct stat info{};if(::stat(path.c_str(),&info)<0)throw Error(ErrorCode::PermissionDenied,"Cannot inspect staging parent");if((info.st_mode&(S_IWGRP|S_IWOTH))&&!(info.st_mode&S_ISVTX))throw Error(ErrorCode::UnsafeEntry,"Archive writes require a private output directory; shared writable ancestors are not supported");}
#else
  (void)parent;
#endif
}
class TemporaryDirectory {
 public:
  fs::path path;
  bool preserve{};
#ifdef _WIN32
  HANDLE directoryHandle=INVALID_HANDLE_VALUE;
#endif
  explicit TemporaryDirectory(const fs::path& parent) {
    fs::create_directories(parent);
    const auto physical=fs::canonical(parent);requirePrivateAncestors(physical);
    std::random_device random;
    for(int n=0;n<30;++n){
      auto candidate=physical/(".reazip-txn-"+std::to_string(random())+"-"+std::to_string(random()));
      if(privateDirectory(candidate)){path=candidate;
#ifdef _WIN32
        directoryHandle=CreateFileW(path.c_str(),FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(directoryHandle==INVALID_HANDLE_VALUE)throw Error(ErrorCode::PermissionDenied,"Cannot lock private staging directory");
#endif
        return;}
    }
    throw Error(ErrorCode::IoError,"Could not create private staging directory");
  }
  ~TemporaryDirectory(){
#ifdef _WIN32
    if(directoryHandle!=INVALID_HANDLE_VALUE)CloseHandle(directoryHandle);
#endif
    if(!path.empty()&&!preserve){std::error_code error;fs::remove_all(path,error);}}
  TemporaryDirectory(const TemporaryDirectory&)=delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&)=delete;
};
}
