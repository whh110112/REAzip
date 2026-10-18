#include <zenzip/engine.hpp>
#include "../../engine/write/journal.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
namespace fs=zenzip::fs;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char** argv){try{
  require(argc==4,"library/work/mode required");const auto work=fs::absolute(fs::u8path(argv[2]));fs::create_directories(work);const std::string mode=argv[3];zenzip::Engine engine(fs::absolute(fs::u8path(argv[1])));
  const auto input=work/"input.txt",archive=work/"recover.zip";zenzip::CreateOptions options;options.output=archive;options.inputs={input};
  zenzip::Callbacks cb;cb.overwrite=[](const auto&){return zenzip::Overwrite::Overwrite;};
  if(mode=="init"){std::ofstream(input)<<"original";engine.create(options,cb);}
  else if(mode=="crash"||mode=="rollback"||mode=="complete"){
    std::ofstream(input)<<"updated content";
    cb.outputCommitted=[&](auto){if(mode=="crash")std::_Exit(86);if(mode=="rollback")throw zenzip::Error(zenzip::ErrorCode::IoError,"simulated commit interruption");};
    cb.commitState=[&](bool committing){if(!committing&&mode=="complete")std::_Exit(88);};
    if(mode=="rollback"){bool failed=false;try{engine.create(options,cb);}catch(const zenzip::Error&){failed=true;}require(failed,"injected failure not observed");}else engine.create(options,cb);
  }else if(mode=="split"){
    const auto bulk=work/"bulk.bin";{std::ofstream f(bulk,std::ios::binary);const std::string block(65536,'x');for(int i=0;i<128;++i)f<<block;}
    options.inputs={bulk};options.output=work/"split.7z";options.format="7z";options.level=0;options.volumeSize=1024*1024;cb.outputCommitted=[](auto){std::_Exit(87);};engine.create(options,cb);
  }else if(mode=="verify"||mode=="verify-complete"){
    // list() itself must recover, without an external recovery command.
    engine.list(archive);engine.test(archive);zenzip::ExtractOptions extract;extract.destination=work/"out";extract.overwrite=zenzip::Overwrite::Overwrite;engine.extract(archive,extract);require(read(extract.destination/input.filename())==(mode=="verify"?"original":"updated content"),"recovered archive content mismatch");
    for(const auto& file:fs::directory_iterator(work))require(file.path().filename().u8string().find(".zenzip-txn-")!=0,"recovered journal directory was not cleaned");
  }else if(mode=="verify-split"){
    zenzip::recoverTransactions(work/"split.7z",{});for(const auto& file:fs::directory_iterator(work))require(file.path().filename().u8string().find("split.7z")!=0,"partial volume remained after recovery");require(fs::is_regular_file(work/"bulk.bin"),"source disappeared during recovery");
  }else if(mode=="verify-external"){
    std::ofstream(archive)<<"external changes";bool blocked=false;try{engine.list(archive);}catch(const zenzip::Error&){blocked=true;}require(blocked&&read(archive)=="external changes","recovery overwrote a file changed after interruption");bool backup=false;for(const auto& p:fs::directory_iterator(work))if(p.is_directory()&&fs::exists(p.path()/"journal.bin"))backup=true;require(backup,"recovery did not retain original backup");
  }else return 2;
  std::cout<<"journal "<<mode<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
