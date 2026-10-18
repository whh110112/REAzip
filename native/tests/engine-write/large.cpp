#include <zenzip/engine.hpp>
#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif
int main(int argc,char** argv){try{if(argc!=3)return 2;namespace fs=zenzip::fs;const auto root=fs::absolute(fs::u8path(argv[2]));fs::create_directories(root);const auto file=root/"over-4GiB.bin";constexpr std::uint64_t size=0x100000000ULL+4096;
#ifdef _WIN32
  const auto handle=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(handle==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create large source");DWORD returned{};if(!DeviceIoControl(handle,FSCTL_SET_SPARSE,nullptr,0,nullptr,0,&returned,nullptr)){CloseHandle(handle);throw std::runtime_error("Sparse source is unavailable on this filesystem");}LARGE_INTEGER position{};position.QuadPart=size-1;DWORD written{};if(!SetFilePointerEx(handle,position,nullptr,FILE_BEGIN)||!WriteFile(handle,"Z",1,&written,nullptr)||written!=1){CloseHandle(handle);throw std::runtime_error("Sparse source write failed");}CloseHandle(handle);
#else
  {std::ofstream source(file,std::ios::binary);source.seekp(size-1);source.put('Z');if(!source)throw std::runtime_error("Sparse source write failed");}
#endif
  zenzip::Engine engine(fs::absolute(fs::u8path(argv[1])));zenzip::CreateOptions options;options.output=root/"large.zip";options.inputs={file};options.level=1;engine.create(options);const auto listing=engine.list(options.output);if(listing.entries.size()!=1||listing.entries[0].size!=size)throw std::runtime_error("ZIP64 size truncated");if(fs::file_size(options.output)>64*1024*1024)throw std::runtime_error("Large test exceeded archive storage budget");std::cout<<"ZIP64 >4GiB native create/list/verify PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
