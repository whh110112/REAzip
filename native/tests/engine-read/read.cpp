#include <reazip/engine.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
  require(argc==4,"expected library/work/fixtures");
  reazip::Engine engine(fs::absolute(fs::u8path(argv[1])));
  const auto work=fs::absolute(fs::u8path(argv[2]));fs::create_directories(work);
  const auto fixtures=fs::absolute(fs::u8path(argv[3]));
  require(engine.list(fixtures/"sample.iso").format=="iso","ISO detection");
  const auto source=work/fs::u8path(u8"中文 😀.txt");std::ofstream(source)<<"test";
  bit7z::Bit7zLibrary lib(fs::absolute(fs::u8path(argv[1])).native());
  const auto enc=work/"header.7z";fs::remove(enc);
  bit7z::BitFileCompressor c(lib,bit7z::BitFormat::SevenZip);
  c.setPassword(BIT7Z_STRING("test-only"),bit7z::EncryptionScope::DataAndHeaders);
  c.compressFiles(std::vector<bit7z::tstring>{source.native()},enc.native());
  bool required=false,wrong=false;
  try{engine.list(enc);}catch(const reazip::Error& e){required=e.code==reazip::ErrorCode::PasswordRequired;}
  try{engine.list(enc,"incorrect");}catch(const reazip::Error& e){wrong=e.code==reazip::ErrorCode::WrongPassword;}
  require(required,"header encryption must report password required without blocking");
  require(wrong,"wrong password classification");
  const auto read=engine.list(enc,"test-only");require(read.entries.size()==1,"entry count");
  require(read.entries[0].path==source.filename().u8string(),"Unicode filename roundtrip");
  engine.test(enc,"test-only");
  for(const bool aes:{false,true}){
    const auto zip=work/(aes?"aes.zip":"legacy.zip");fs::remove(zip);
    bit7z::BitFileCompressor z(lib,bit7z::BitFormat::Zip);z.setPassword(BIT7Z_STRING("test-only"));
    if(aes)z.setFormatProperty(L"em",L"AES256");else z.setFormatProperty(L"em",L"ZipCrypto");
    z.compressFiles(std::vector<bit7z::tstring>{source.native()},zip.native());
    require(engine.list(zip).entries[0].encryption==(aes?"AES":"ZipCrypto"),"encryption metadata must be truthful");
  }
  require(engine.list(fixtures/"gbk.zip","","936").entries[0].path==u8"中文.txt","GBK name decoding");
  bool cancelled=false;reazip::Callbacks cb;cb.cancelled=[](){return true;};
  try{engine.list(enc,"test-only",{},cb);}catch(const reazip::Error& e){cancelled=e.code==reazip::ErrorCode::Cancelled;}
  require(cancelled,"pre-operation cancellation");
  std::cout<<engine.identity()<<"\nNative read regressions PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
