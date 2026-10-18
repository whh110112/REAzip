#include <zenzip/engine.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
namespace fs=zenzip::fs;
static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
  require(argc==4,"expected library/work/fixtures");
  zenzip::Engine engine(fs::absolute(fs::u8path(argv[1])));
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
  try{engine.list(enc);}catch(const zenzip::Error& e){required=e.code==zenzip::ErrorCode::PasswordRequired;}
  try{engine.list(enc,"incorrect");}catch(const zenzip::Error& e){wrong=e.code==zenzip::ErrorCode::WrongPassword;}
  require(required,"header encryption must report password required without blocking");
  require(wrong,"wrong password classification");
  const auto read=engine.list(enc,"test-only");require(read.entries.size()==1,"entry count");
  require(read.entries[0].path==source.filename().u8string(),"Unicode filename roundtrip");
  engine.test(enc,"test-only");
  for(const auto& name:{"test_read_format_rar.rar","test_read_format_rar5_stored.rar"}){
    const auto path=fixtures/name;const auto listing=engine.list(path);require(listing.format=="rar"||listing.format=="rar5","RAR/RAR5 detection");const auto item=std::find_if(listing.entries.begin(),listing.entries.end(),[](const auto& e){return !e.directory&&!e.link&&e.size>0;});require(item!=listing.entries.end(),"RAR fixture has no regular payload");zenzip::ExtractOptions extract;extract.destination=work/name;extract.entryIds={item->id};extract.overwrite=zenzip::Overwrite::Overwrite;engine.extract(path,extract);require(fs::file_size(extract.destination/fs::u8path(item->path))==item->size,"RAR/RAR5 native extraction");
  }
  const auto wim=work/"original-fixture.wim";fs::remove(wim);bit7z::BitFileCompressor image(lib,bit7z::BitFormat::Wim);image.compressFiles(std::vector<bit7z::tstring>{source.native()},wim.native());const auto imageList=engine.list(wim);require(imageList.format=="wim","complete engine WIM support");zenzip::ExtractOptions imageExtract;imageExtract.destination=work/"wim-output";imageExtract.overwrite=zenzip::Overwrite::Overwrite;engine.extract(wim,imageExtract);bool imageBytes=false;for(const auto& item:fs::recursive_directory_iterator(imageExtract.destination))if(item.is_regular_file()&&fs::file_size(item.path())==4){std::ifstream stream(item.path());std::string text;stream>>text;if(text=="test")imageBytes=true;}require(imageBytes,"WIM extraction content");
  for(const bool aes:{false,true}){
    const auto zip=work/(aes?"aes.zip":"legacy.zip");fs::remove(zip);
    bit7z::BitFileCompressor z(lib,bit7z::BitFormat::Zip);z.setPassword(BIT7Z_STRING("test-only"));
    if(aes)z.setFormatProperty(L"em",L"AES256");else z.setFormatProperty(L"em",L"ZipCrypto");
    z.compressFiles(std::vector<bit7z::tstring>{source.native()},zip.native());
    require(engine.list(zip).entries[0].encryption==(aes?"AES":"ZipCrypto"),"encryption metadata must be truthful");
  }
  require(engine.list(fixtures/"gbk.zip","","936").entries[0].path==u8"中文.txt","GBK name decoding");
  require(engine.list(fixtures/"big5.zip","","950").entries[0].path==u8"繁體中文.txt","Big5 name decoding");
  require(engine.list(fixtures/"shiftjis.zip","","932").entries[0].path==u8"日本語.txt","Shift-JIS name decoding");
  // Same path/size/mtime must not reuse old entry IDs after a directory change.
  const auto first=work/"item-a.txt",second=work/"item-b.txt",stale=work/"stale.zip";std::ofstream(first)<<"payload";std::ofstream(second)<<"payload";fs::remove(stale);bit7z::BitFileCompressor stored(lib,bit7z::BitFormat::Zip);stored.setCompressionLevel(bit7z::BitCompressionLevel::None);stored.compressFiles(std::vector<bit7z::tstring>{first.native()},stale.native());const auto old=engine.list(stale);const auto stamp=fs::last_write_time(stale);const auto oldSize=fs::file_size(stale);fs::remove(stale);stored.compressFiles(std::vector<bit7z::tstring>{second.native()},stale.native());require(fs::file_size(stale)==oldSize,"stale ID fixture must preserve size");fs::last_write_time(stale,stamp);require(engine.list(stale).revision!=old.revision,"directory fingerprint must invalidate preserved metadata");bool expired=false;try{engine.remove(stale,{old.entries[0].id});}catch(const zenzip::Error&){expired=true;}require(expired&&engine.list(stale).entries[0].path=="item-b.txt","stale selection deleted a different file");
  bool cancelled=false;zenzip::Callbacks cb;cb.cancelled=[](){return true;};
  try{engine.list(enc,"test-only",{},cb);}catch(const zenzip::Error& e){cancelled=e.code==zenzip::ErrorCode::Cancelled;}
  require(cancelled,"pre-operation cancellation");
  std::cout<<engine.identity()<<"\nNative read regressions PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
