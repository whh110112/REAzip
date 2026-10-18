#include "../../engine/write/secure-output.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif
namespace fs=zenzip::fs;
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void linkDirectory(const fs::path& target,const fs::path& path){
#ifdef _WIN32
  fs::create_directory(path);auto handle=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);require(handle!=INVALID_HANDLE_VALUE,"open test junction");
  struct Reparse {DWORD tag;WORD length,reserved,subOffset,subLength,printOffset,printLength;wchar_t buffer[2048];} data{};
  const auto substitute=std::wstring(L"\\??\\")+target.native(),print=target.native();data.tag=IO_REPARSE_TAG_MOUNT_POINT;data.subLength=static_cast<WORD>(substitute.size()*2);data.printOffset=data.subLength+2;data.printLength=static_cast<WORD>(print.size()*2);data.length=8+data.subLength+2+data.printLength+2;std::copy(substitute.begin(),substitute.end(),data.buffer);std::copy(print.begin(),print.end(),data.buffer+substitute.size()+1);DWORD bytes{};const auto ok=DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,&data,data.length+8,nullptr,0,&bytes,nullptr);CloseHandle(handle);require(ok,"create unprivileged NTFS test junction");
#else
  fs::create_directory_symlink(target,path);
#endif
}
int main(int argc,char** argv){try{
  require(argc==2,"work directory required");const auto work=fs::absolute(fs::u8path(argv[1]))/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());const auto root=work/"root",outside=work/"outside",moved=work/"moved";fs::create_directories(root);fs::create_directory(outside);
  {
    zenzip::SecureOutput output(root);output.directory("dir");bool movedParent=false;
    zenzip::Callbacks callbacks;callbacks.outputFileBeforeCommit=[&](const auto&){std::error_code error;fs::rename(root/"dir",moved,error);if(!error){movedParent=true;linkDirectory(outside,root/"dir");}};
    bool blocked=false;try{output.file(fs::path("dir")/"data.txt",false,[](auto& stream){stream<<"archive bytes";},callbacks);}catch(const zenzip::Error&){blocked=true;}
#ifdef _WIN32
    require(!movedParent&&fs::is_regular_file(root/"dir/data.txt"),"directory handles must prevent parent rename on Windows");
#else
    require(movedParent&&blocked&&!fs::exists(moved/"data.txt"),"moved/replaced parent must be rejected before commit");
#endif
    require(fs::is_empty(outside),"directory replacement wrote outside destination");
  }
  const auto root2=work/"linked-root";fs::create_directory(root2);linkDirectory(outside,root2/"link");
  {zenzip::SecureOutput output(root2);bool blocked=false;try{output.file(fs::path("link")/"escape.txt",false,[](auto& stream){stream<<"not allowed";},{});}catch(const zenzip::Error&){blocked=true;}require(blocked&&fs::is_empty(outside),"existing junction/symlink followed");}
  const auto root3=work/"collision";fs::create_directory(root3);
  {zenzip::SecureOutput output(root3);zenzip::Callbacks cb;cb.outputFileBeforeCommit=[&](const auto&){std::ofstream(root3/"keep.txt")<<"external";};bool refused=false;try{output.file("keep.txt",false,[](auto& stream){stream<<"new";},cb);}catch(const zenzip::Error&){refused=true;}require(refused,"late file collision must not overwrite without permission");std::ifstream file(root3/"keep.txt");std::string text;file>>text;require(text=="external","late collision destroyed external data");}
  std::cout<<"anchored directory handles, concurrent parent replacement, links/junctions, late collision PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
