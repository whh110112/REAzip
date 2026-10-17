#include "internal.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
namespace reazip {
void checkCancelled(const Callbacks& cb) { if(cb.cancelled && cb.cancelled()) throw Error(ErrorCode::Cancelled,"Task cancelled"); }
void attach(bit7z::BitAbstractArchiveHandler& handler,const Callbacks& cb) {
  auto total=std::make_shared<std::uint64_t>(0);
  handler.setTotalCallback([total](std::uint64_t n){*total=n;});
  handler.setProgressCallback([cb,total](std::uint64_t n){if(cb.progress)cb.progress(n,*total);return !(cb.cancelled&&cb.cancelled());});
}
[[noreturn]] void translate(const bit7z::BitException& e,const std::string& password,const Callbacks& cb) {
  checkCancelled(cb);
  if(e.code()==bit7z::BitFailureSource::WrongPassword)
    throw Error(password.empty()?ErrorCode::PasswordRequired:ErrorCode::WrongPassword,"Archive password required or incorrect");
  if(e.code()==bit7z::BitFailureSource::OperationNotSupported)
    throw Error(ErrorCode::UnsupportedOperation,"Operation not supported for this archive");
  if(e.code()==bit7z::BitFailureSource::OperationNotPermitted)
    throw Error(ErrorCode::PermissionDenied,"File access denied");
  throw Error(ErrorCode::CorruptArchive,std::string("Archive operation failed (")+e.code().category().name()+":"+std::to_string(e.code().value())+")");
}
std::string revision(const fs::path& p) {
  const auto text=fs::weakly_canonical(p).u8string()+":"+std::to_string(fs::file_size(p))+":"+std::to_string(fs::last_write_time(p).time_since_epoch().count());
  std::uint64_t value=14695981039346656037ull;
  for(unsigned char c:text){value^=c;value*=1099511628211ull;}
  std::ostringstream s;s<<std::hex<<value;return s.str();
}
std::string formatName(const bit7z::BitInFormat& f) {
  using namespace bit7z;
  if(f==BitFormat::Zip)return "zip";if(f==BitFormat::SevenZip)return "7z";
  if(f==BitFormat::Tar)return "tar";if(f==BitFormat::GZip)return "gzip";
  if(f==BitFormat::Rar)return "rar";if(f==BitFormat::Rar5)return "rar5";
  if(f==BitFormat::Iso)return "iso";if(f==BitFormat::Wim)return "wim";
  return "format-"+std::to_string(f.value());
}
std::unique_ptr<bit7z::BitArchiveReader> openReader(bit7z::Bit7zLibrary& lib,const fs::path& p,const std::string& pw,const std::string& cp,const Callbacks& cb) {
  checkCancelled(cb);
  try {
    auto reader=std::make_unique<bit7z::BitArchiveReader>(lib,ns(fs::absolute(p)),bit7z::BitFormat::Auto,ns(pw));
    if(!cp.empty()) {
      if(cp!="936"&&cp!="950"&&cp!="932"&&cp!="65001")throw Error(ErrorCode::UnsupportedOperation,"Unsupported codepage");
      if(reader->detectedFormat()==bit7z::BitFormat::Zip)reader->useFormatProperty(L"cp",static_cast<std::uint32_t>(std::stoul(cp)));
    }
    attach(*reader,cb);
    return reader;
  }catch(const bit7z::BitException& e){translate(e,pw,cb);}
}
Engine::Engine(const fs::path& p) {
  if(!p.is_absolute()||!fs::is_regular_file(p))throw Error(ErrorCode::EngineMissing,"Engine must be an existing absolute library path");
  try{impl=std::make_unique<Impl>(p);}catch(const std::exception&){throw Error(ErrorCode::EngineArchitectureMismatch,"Native engine could not load; check architecture and runtime dependencies");}
}
Engine::~Engine()=default;
std::string Engine::identity() const {
  std::string arch;
#if defined(__aarch64__) || defined(_M_ARM64)
  arch="arm64";
#elif defined(__x86_64__) || defined(_M_X64)
  arch="x64";
#else
  arch="unsupported";
#endif
  return "7-Zip 26.04 / bit7z 4.1.0 / "+arch+" / "+impl->path.u8string();
}
Listing Engine::list(const fs::path& p,const std::string& pw,const std::string& cp,const Callbacks& cb) {
  auto reader=openReader(impl->library,p,pw,cp,cb);
  Listing result;result.revision=revision(p);result.format=formatName(reader->detectedFormat());result.physicalSize=fs::file_size(p);
  for(const auto& item:*reader) {
    checkCancelled(cb);Entry e;e.index=item.index();e.id=result.revision+":"+std::to_string(e.index);
    e.path=utf8(item.path());std::replace(e.path.begin(),e.path.end(),'\\','/');
    e.directory=item.isDir();e.encrypted=item.isEncrypted();e.link=item.isSymLink();e.size=item.size();
    const auto packed=item.itemProperty(bit7z::BitProperty::PackSize);
    if(packed.isUInt64())e.packed=packed.getUInt64();
    auto hard=item.itemProperty(bit7z::BitProperty::HardLink);if(!hard.isEmpty())e.link=true;
    const auto method=item.itemProperty(bit7z::BitProperty::Method);
    if(!e.encrypted)e.encryption="none";
    else if(method.isString()) {
      const auto text=utf8(method.toString());
      if(text.find("AES")!=std::string::npos)e.encryption="AES";
      else if(text.find("ZipCrypto")!=std::string::npos)e.encryption="ZipCrypto";
    }
    result.entries.push_back(std::move(e));
  }
  if(revision(p)!=result.revision)throw Error(ErrorCode::IoError,"Archive changed while reading");
  return result;
}
void Engine::test(const fs::path& p,const std::string& pw,const Callbacks& cb) {
  try{auto reader=openReader(impl->library,p,pw,{},cb);reader->test();}catch(const bit7z::BitException& e){translate(e,pw,cb);}
}
}
