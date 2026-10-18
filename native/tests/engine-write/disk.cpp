#include <zenzip/engine.hpp>
#include <fstream>
#include <iostream>
#include <random>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#include <csignal>
#endif
namespace fs=zenzip::fs;
std::string bytes(const fs::path& p){std::ifstream file(p,std::ios::binary);return {(std::istreambuf_iterator<char>(file)),{}};}
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
  require(argc==5,"library/work/output/mode");const auto work=fs::absolute(fs::u8path(argv[2])),output=fs::absolute(fs::u8path(argv[3]));const std::string mode=argv[4];fs::create_directories(work);fs::create_directories(output);zenzip::Engine engine(fs::absolute(fs::u8path(argv[1])));
  const auto source=work/"source.txt",random=work/"random.bin",archive=output/"existing.zip";std::ofstream(source)<<"old contents";std::mt19937 generator(27);{std::ofstream file(random,std::ios::binary);std::string block(65536,0);for(int n=0;n<64;++n){for(auto& c:block)c=static_cast<char>(generator());file.write(block.data(),block.size());}}
  zenzip::CreateOptions options;options.inputs={source};options.output=archive;options.level=0;zenzip::Callbacks cb;cb.overwrite=[](const auto&){return zenzip::Overwrite::Overwrite;};engine.create(options,cb);const auto original=bytes(archive);options.inputs={random};
#ifdef _WIN32
  auto lock=CreateFileW(archive.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(lock!=INVALID_HANDLE_VALUE,"archive lock unavailable");
#else
  struct rlimit saved{};if(mode=="limit"){getrlimit(RLIMIT_FSIZE,&saved);auto limited=saved;limited.rlim_cur=2*1024*1024;std::signal(SIGXFSZ,SIG_IGN);require(setrlimit(RLIMIT_FSIZE,&limited)==0,"cannot set file size limit");}
#endif
  std::cerr<<"disk-test: creating archive into constrained filesystem\n";bool refused=false;try{engine.create(options,cb);}catch(const std::exception& e){std::cerr<<"disk-test: refused: "<<e.what()<<'\n';refused=true;}
#ifdef _WIN32
  CloseHandle(lock);
#else
  if(mode=="limit")require(setrlimit(RLIMIT_FSIZE,&saved)==0,"cannot restore file size limit");
#endif
  std::cerr<<"disk-test: validating preserved archive\n";require(refused&&bytes(archive)==original&&fs::file_size(random)==4*1024*1024,"I/O failure damaged existing archive or source");engine.test(archive);
  if(mode=="full"){
    std::cerr<<"disk-test: preparing unconstrained extraction source\n";zenzip::CreateOptions large;large.inputs={random};large.output=work/"large.zip";large.level=0;engine.create(large);zenzip::ExtractOptions extract;extract.destination=output/"extract";extract.overwrite=zenzip::Overwrite::Overwrite;fs::create_directory(extract.destination);const auto keep=extract.destination/random.filename();std::ofstream(keep)<<"keep";std::cerr<<"disk-test: extracting into constrained filesystem\n";refused=false;try{engine.extract(large.output,extract);}catch(const std::exception& e){std::cerr<<"disk-test: extraction refused: "<<e.what()<<'\n';refused=true;}require(refused&&bytes(keep)=="keep","full extraction filesystem destroyed existing target");
  }
  std::cout<<"actual OS write/commit refusal preserves original archive, destination and sources PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
