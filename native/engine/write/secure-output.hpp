#pragma once
#include <zenzip/types.hpp>
#include <memory>
#include <ostream>
namespace zenzip {
// Directory handles anchor every component; archive names never resolve through links.
class SecureOutput {
 public:
  explicit SecureOutput(const fs::path& root);
  ~SecureOutput();
  void directory(const fs::path& relative);
  void file(const fs::path& relative,bool replace,const std::function<void(std::ostream&)>& writer,const Callbacks&);
 private:struct Impl;std::unique_ptr<Impl> impl;
};
}
