#include "../read/internal.hpp"
#include "../read/temporary.hpp"
#include <bit7z/bitfilecompressor.hpp>
#include <bit7z/bitarchiveeditor.hpp>
#include <fstream>
#include <set>
#include <map>
#include <regex>
#include <algorithm>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace reazip {
namespace {
struct CommitGuard { const Callbacks& cb; explicit CommitGuard(const Callbacks& c):cb(c){if(cb.commitState)cb.commitState(true);} ~CommitGuard(){if(cb.commitState)cb.commitState(false);} };
fs::path safeRelative(std::string text) {
  std::replace(text.begin(),text.end(),'\\','/');
  const auto p=fs::u8path(text);
  if(text.empty()||text.front()=='/'||p.has_root_name()||p.has_root_directory()||text.find(':')!=std::string::npos||text.find('\0')!=std::string::npos)
    throw Error(ErrorCode::UnsafeEntry,"Absolute or invalid archive path");
  const std::regex devices("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\\..*)?$",std::regex::icase);
  for(const auto& c:p){const auto s=c.u8string();if(s==".."||s=="."||s.empty()||s.back()=='.'||s.back()==' '||std::regex_match(s,devices))throw Error(ErrorCode::UnsafeEntry,"Unsafe archive path component");}
  return p;
}
void verifyParents(const fs::path& root,const fs::path& relative) {
  auto p=root;
  for(const auto& c:relative){
    p/=c;std::error_code error;const auto st=fs::symlink_status(p,error);
    if(error && error!=std::errc::no_such_file_or_directory)throw Error(ErrorCode::PermissionDenied,"Destination cannot be inspected");
    if(fs::is_symlink(st))throw Error(ErrorCode::UnsafeEntry,"Destination contains a symbolic link");
#ifdef _WIN32
    const auto attrs=GetFileAttributesW(p.c_str());
    if(attrs!=INVALID_FILE_ATTRIBUTES && (attrs&FILE_ATTRIBUTE_REPARSE_POINT))throw Error(ErrorCode::UnsafeEntry,"Destination contains a reparse point");
#endif
  }
}
void replaceFile(const fs::path& source,const fs::path& target) {
#ifdef _WIN32
  if(!MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw Error(ErrorCode::IoError,"Could not commit output file");
#else
  fs::rename(source,target);
#endif
}
fs::path chooseTarget(fs::path target,Overwrite policy,const Callbacks& cb) {
  if(!fs::exists(target))return target;
  if(fs::is_directory(target))throw Error(ErrorCode::IoError,"Destination is a directory");
  if(policy==Overwrite::Ask){if(!cb.overwrite)throw Error(ErrorCode::UnsupportedOperation,"Overwrite decision required");policy=cb.overwrite(target);}
  checkCancelled(cb);
  if(policy==Overwrite::Ask)throw Error(ErrorCode::Cancelled,"Overwrite dialog cancelled");
  if(policy==Overwrite::Skip)return {};
  if(policy==Overwrite::Rename){
    for(std::uint64_t i=1;;++i){auto alternative=target.parent_path()/(target.stem().u8string()+" ("+std::to_string(i)+")"+target.extension().u8string());if(!fs::exists(alternative))return alternative;}
  }
  return target;
}
const bit7z::BitInOutFormat& formatFor(const std::string& name) {
  if(name=="zip")return bit7z::BitFormat::Zip;if(name=="7z")return bit7z::BitFormat::SevenZip;
  if(name=="tar")return bit7z::BitFormat::Tar;if(name=="gzip")return bit7z::BitFormat::GZip;
  throw Error(ErrorCode::UnsupportedOperation,"Archive format does not support this write operation");
}
std::vector<std::uint32_t> selected(const Listing& l,const std::vector<std::string>& ids) {
  std::set<std::uint32_t> result;
  if(ids.empty()){for(const auto& e:l.entries)result.insert(e.index);}
  for(const auto& id:ids){
    bool found=false;
    for(const auto& e:l.entries)if(e.id==id){found=true;result.insert(e.index);if(e.directory)for(const auto& c:l.entries)if(c.path.compare(0,e.path.size()+1,e.path+"/")==0)result.insert(c.index);}
    const auto prefix=l.revision+":dir:";
    if(id.compare(0,prefix.size(),prefix)==0){const auto folder=id.substr(prefix.size())+"/";for(const auto& e:l.entries)if(e.path.compare(0,folder.size(),folder)==0){found=true;result.insert(e.index);}}
    if(!found)throw Error(ErrorCode::IoError,"Selection expired or does not exist");
  }
  return {result.begin(),result.end()};
}
std::uint64_t fingerprint(const fs::path& p) {
  std::ifstream in(p,std::ios::binary);if(!in)throw Error(ErrorCode::PermissionDenied,"Input cannot be read");
  std::uint64_t hash=14695981039346656037ull;char buffer[65536];
  while(in){in.read(buffer,sizeof(buffer));for(std::streamsize i=0;i<in.gcount();++i){hash^=static_cast<unsigned char>(buffer[i]);hash*=1099511628211ull;}}
  if(!in.eof())throw Error(ErrorCode::IoError,"Input read failed");return hash;
}
using Aliases=std::vector<std::pair<bit7z::tstring,bit7z::tstring>>;
Aliases gather(const std::vector<fs::path>& inputs,const fs::path& output) {
  Aliases aliases;std::set<std::string> names;
  auto add=[&](const fs::path& file,const fs::path& alias){
    if(fs::is_symlink(fs::symlink_status(file)))throw Error(ErrorCode::UnsafeEntry,"Symbolic-link inputs are not supported");
    safeRelative(alias.generic_u8string());
    if(!names.insert(alias.generic_u8string()).second)throw Error(ErrorCode::IoError,"Duplicate input archive path");
    aliases.emplace_back(ns(fs::absolute(file)),ns(alias));
  };
  for(const auto& raw:inputs){
    const auto p=fs::absolute(raw);if(!fs::exists(p))throw Error(ErrorCode::IoError,"Input does not exist");
    if(fs::is_directory(p)){
      if(fs::is_symlink(fs::symlink_status(p)))throw Error(ErrorCode::UnsafeEntry,"Linked input directory");
      bool any=false;
      for(const auto& item:fs::recursive_directory_iterator(p)){
        if(item.path()==output)continue;
        if(item.is_symlink())throw Error(ErrorCode::UnsafeEntry,"Linked input entry");
        if(item.is_regular_file()){add(item.path(),p.filename()/fs::relative(item.path(),p));any=true;}
        else if(item.is_directory()&&fs::is_empty(item.path())){add(item.path(),p.filename()/fs::relative(item.path(),p));any=true;}
      }
      if(!any)add(p,p.filename());
    }else {if(p==output)throw Error(ErrorCode::IoError,"Output is also an input");add(p,p.filename());}
  }
  return aliases;
}
void configureCreator(bit7z::BitAbstractArchiveCreator& creator,const CreateOptions& options,const Callbacks& cb) {
  if(options.level<0||options.level>9)throw Error(ErrorCode::UnsupportedOperation,"Invalid compression level");
  creator.setCompressionLevel(static_cast<bit7z::BitCompressionLevel>(options.level));
  if(!options.password.empty())creator.setPassword(ns(options.password),options.encryptHeaders?bit7z::EncryptionScope::DataAndHeaders:bit7z::EncryptionScope::DataOnly);
  if(options.format=="zip"&&!options.password.empty())creator.setFormatProperty(L"em",options.aes?L"AES256":L"ZipCrypto");
  creator.setVolumeSize(options.volumeSize);attach(creator,cb);
}
}
void Engine::extract(const fs::path& source,const ExtractOptions& options,const Callbacks& cb) {
  try{
    const auto listing=list(source,options.password,options.codepage,cb);
    if(options.password.empty()&&std::any_of(listing.entries.begin(),listing.entries.end(),[](const auto& e){return e.encrypted;}))throw Error(ErrorCode::PasswordRequired,"Archive password required");
    auto reader=openReader(impl->library,source,options.password,options.codepage,cb);
    const auto dest=fs::absolute(options.destination);
    fs::create_directories(dest);const auto root=fs::canonical(dest);
    TemporaryDirectory staging(root);
    if(listing.format=="gzip" && listing.entries.size()==1 && fs::u8path(listing.entries.front().path).extension()==".tar") {
      if(!options.entryIds.empty())throw Error(ErrorCode::UnsupportedOperation,"Select files after opening the nested TAR");
      const auto tar=staging.path/"nested.tar";std::ofstream stream(tar,std::ios::binary);reader->extractTo(stream,0);stream.close();
      if(!stream)throw Error(ErrorCode::IoError,"Could not spool nested TAR");
      auto nested=options;nested.entryIds.clear();extract(tar,nested,cb);return;
    }
    for(auto index:selected(listing,options.entryIds)){
      checkCancelled(cb);const auto& entry=listing.entries.at(index);
      if(entry.link)throw Error(ErrorCode::UnsafeEntry,"Links inside archives are not extracted");
      const auto relative=safeRelative(entry.path);verifyParents(root,relative);
      auto target=root/relative;
      if(entry.directory){fs::create_directories(target);continue;}
      target=chooseTarget(target,options.overwrite,cb);if(target.empty())continue;
      fs::create_directories(target.parent_path());verifyParents(root,fs::relative(target,root));
      const auto temp=staging.path/std::to_string(index);
      std::ofstream stream(temp,std::ios::binary);if(!stream)throw Error(ErrorCode::IoError,"Could not create temporary output");
      reader->extractTo(stream,index);stream.close();if(!stream)throw Error(ErrorCode::IoError,"Extracted file could not be written");
      checkCancelled(cb);verifyParents(root,fs::relative(target,root));{CommitGuard commit(cb);replaceFile(temp,target);}
    }
  }catch(const bit7z::BitException& e){translate(e,options.password,cb);}
}
std::vector<fs::path> Engine::create(const CreateOptions& options,const Callbacks& cb) {
  checkCancelled(cb);
  if(options.inputs.empty())throw Error(ErrorCode::IoError,"Select input files");
  if((options.format=="tar"||options.format=="tgz")&&(!options.password.empty()||options.encryptHeaders))throw Error(ErrorCode::UnsupportedOperation,"TAR/TGZ does not support encryption");
  if(options.encryptHeaders&&options.format!="7z")throw Error(ErrorCode::UnsupportedOperation,"Header encryption requires 7z");
  if(options.deleteSources&&!cb.trash)throw Error(ErrorCode::UnsupportedOperation,"Recoverable source deletion is unavailable");
  const auto output=fs::absolute(options.output);const auto aliases=gather(options.inputs,output);
  if(options.volumeSize && fs::exists(output.parent_path()))for(const auto& old:fs::directory_iterator(output.parent_path()))
    if(old.path().filename().u8string().compare(0,output.filename().u8string().size()+1,output.filename().u8string()+".")==0)
      throw Error(ErrorCode::UnsupportedOperation,"Choose a new output name for a split archive; replacing an existing volume set is not supported");
  std::map<fs::path,std::uint64_t> original;
  if(options.deleteSources)for(const auto& pair:aliases)if(fs::is_regular_file(fs::path(pair.first)))original.emplace(fs::path(pair.first),fingerprint(fs::path(pair.first)));
  TemporaryDirectory staging(output.parent_path());const auto temp=staging.path/output.filename();
  try{
    if(options.format=="tgz"){
      if(options.volumeSize)throw Error(ErrorCode::UnsupportedOperation,"TGZ volumes are not supported");
      const auto tar=staging.path/"contents.tar";bit7z::BitFileCompressor first(impl->library,bit7z::BitFormat::Tar);attach(first,cb);first.compress(aliases,ns(tar));
      bit7z::BitFileCompressor second(impl->library,bit7z::BitFormat::GZip);attach(second,cb);second.compressFiles(std::vector<bit7z::tstring>{ns(tar)},ns(temp));fs::remove(tar);
    }else{
      bit7z::BitFileCompressor creator(impl->library,formatFor(options.format));configureCreator(creator,options,cb);creator.compress(aliases,ns(temp));
    }
    checkCancelled(cb);
    std::vector<fs::path> files;
    if(fs::exists(temp))files.push_back(temp);
    else for(const auto& item:fs::directory_iterator(staging.path))if(item.is_regular_file()&&item.path().filename().u8string().find(output.filename().u8string()+".")==0)files.push_back(item.path());
    std::sort(files.begin(),files.end());if(files.empty())throw Error(ErrorCode::IoError,"No archive output generated");
    test(files.front(),options.password,cb);
    for(const auto& pair:original)if(fingerprint(pair.first)!=pair.second)throw Error(ErrorCode::IoError,"Source changed; source files retained");
    std::vector<fs::path> outputs;
    for(const auto& file:files){auto target=chooseTarget(output.parent_path()/file.filename(),Overwrite::Ask,cb);if(target.empty())throw Error(ErrorCode::Cancelled,"Output skipped");outputs.push_back(target);}
    std::vector<fs::path> backups(outputs.size());std::size_t committed=0;
    CommitGuard commit(cb);
    try{
      for(std::size_t i=0;i<outputs.size();++i){checkCancelled(cb);if(fs::exists(outputs[i])){backups[i]=staging.path/("backup-"+std::to_string(i));fs::copy_file(outputs[i],backups[i]);}replaceFile(files[i],outputs[i]);++committed;}
    }catch(...){
      for(std::size_t i=0;i<committed;++i){std::error_code error;if(backups[i].empty())fs::remove(outputs[i],error);else {try{replaceFile(backups[i],outputs[i]);}catch(...){staging.preserve=true;}}}
      throw;
    }
    if(options.deleteSources)for(const auto& pair:original){if(fingerprint(pair.first)!=pair.second||!cb.trash(pair.first))throw Error(ErrorCode::IoError,"Archive saved, but some source files could not be moved to trash");}
    return outputs;
  }catch(const bit7z::BitException& e){translate(e,options.password,cb);}
}
namespace {
template<class F> void editArchive(Engine& engine,bit7z::Bit7zLibrary& lib,const fs::path& file,const std::string& password,const Callbacks& cb,F edit){
  auto listing=engine.list(file,password,{},cb);const auto& format=formatFor(listing.format);
  if(listing.format!="zip"&&listing.format!="7z"&&listing.format!="tar")throw Error(ErrorCode::UnsupportedOperation,"Read-only archive format");
  TemporaryDirectory staging(fs::absolute(file).parent_path());const auto copy=staging.path/file.filename();fs::copy_file(file,copy);
  try{
    bool headers=false;
    if(listing.format=="7z"&&!password.empty())try{engine.list(file);}catch(const Error& error){if(error.code==ErrorCode::PasswordRequired)headers=true;else throw;}
    {bit7z::BitArchiveEditor editor(lib,ns(copy),format,ns(password));attach(editor,cb);
      if(!password.empty()){editor.setPassword(ns(password),headers?bit7z::EncryptionScope::DataAndHeaders:bit7z::EncryptionScope::DataOnly);if(listing.format=="zip")editor.setFormatProperty(L"em",L"AES256");}
      edit(editor,listing);editor.applyChanges();}
    engine.test(copy,password,cb);checkCancelled(cb);
    if(revision(file)!=listing.revision)throw Error(ErrorCode::IoError,"Archive changed during update");
    {CommitGuard commit(cb);replaceFile(copy,file);}
  }catch(const bit7z::BitException& e){translate(e,password,cb);}
}
}
void Engine::remove(const fs::path& file,const std::vector<std::string>& ids,const std::string& pw,const Callbacks& cb){
  if(ids.empty())throw Error(ErrorCode::UnsupportedOperation,"Select entries to delete");
  editArchive(*this,impl->library,file,pw,cb,[&](auto& editor,const auto& listing){for(auto i:selected(listing,ids))editor.deleteItem(i,bit7z::DeletePolicy::ItemOnly);});
}
void Engine::rename(const fs::path& file,const std::string& id,const std::string& name,const std::string& pw,const Callbacks& cb){
  const auto renamed=safeRelative(name);
  editArchive(*this,impl->library,file,pw,cb,[&](auto& editor,const auto& listing){
    auto indices=selected(listing,{id});std::string folder;
    for(const auto& e:listing.entries)if(e.id==id&&e.directory)folder=e.path;
    const auto prefix=listing.revision+":dir:";if(id.compare(0,prefix.size(),prefix)==0)folder=id.substr(prefix.size());
    for(auto i:indices){const auto& entry=listing.entries.at(i);
      const auto target=renamed.generic_u8string()+(folder.empty()?std::string():entry.path.substr(folder.size()));
      safeRelative(target);
      for(const auto& e:listing.entries)if(e.path==target&&std::find(indices.begin(),indices.end(),e.index)==indices.end())throw Error(ErrorCode::IoError,"Archive name already exists");
      editor.renameItem(i,ns(target));}
  });
}
void Engine::add(const fs::path& file,const std::vector<fs::path>& inputs,const std::string& pw,const Callbacks& cb){
  const auto aliases=gather(inputs,fs::absolute(file));
  editArchive(*this,impl->library,file,pw,cb,[&](auto& editor,const auto& listing){
    for(const auto& pair:aliases){
      const auto name=utf8(pair.second);std::vector<std::uint32_t> collisions;
      for(const auto& entry:listing.entries)if(entry.path==name)collisions.push_back(entry.index);
      if(collisions.empty()){editor.addItems(Aliases{pair});continue;}
      if(!cb.overwrite)throw Error(ErrorCode::UnsupportedOperation,"Archive overwrite decision required");
      const auto decision=cb.overwrite(fs::u8path(name));checkCancelled(cb);
      if(decision==Overwrite::Ask)throw Error(ErrorCode::Cancelled,"Archive update cancelled");
      if(decision==Overwrite::Skip)continue;
      if(decision==Overwrite::Overwrite){if(collisions.size()!=1)throw Error(ErrorCode::UnsupportedOperation,"Duplicate archive names require an entry-specific update");editor.updateItem(collisions.front(),pair.first);}
      else {const auto base=fs::u8path(name);for(std::uint64_t n=1;;++n){const auto alternative=base.parent_path()/(base.stem().u8string()+" ("+std::to_string(n)+")"+base.extension().u8string());bool exists=false;for(const auto& e:listing.entries)if(e.path==alternative.generic_u8string())exists=true;if(!exists){editor.addItems(Aliases{{pair.first,ns(alternative)}});break;}}}
    }
  });
}
}
