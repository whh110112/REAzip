#pragma once
#include <reazip/types.hpp>
#include <random>
namespace reazip {
class TemporaryDirectory {
 public:
  fs::path path;
  bool preserve{};
  explicit TemporaryDirectory(const fs::path& parent) {
    fs::create_directories(parent);
    std::random_device random;
    for(int n=0;n<30;++n){
      auto candidate=fs::absolute(parent)/(".reazip-txn-"+std::to_string(random())+"-"+std::to_string(random()));
      if(fs::create_directory(candidate)){path=candidate;fs::permissions(path,fs::perms::owner_all,fs::perm_options::replace);return;}
    }
    throw Error(ErrorCode::IoError,"Could not create private staging directory");
  }
  ~TemporaryDirectory(){if(!path.empty()&&!preserve){std::error_code error;fs::remove_all(path,error);}}
  TemporaryDirectory(const TemporaryDirectory&)=delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&)=delete;
};
}
