#pragma once
#include "types.hpp"
#include <memory>
namespace reazip {
class Engine {
 public:
  explicit Engine(const fs::path& library);
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  Listing list(const fs::path&, const std::string& password = {}, const std::string& codepage = {}, const Callbacks& = {});
  void test(const fs::path&, const std::string& password = {}, const Callbacks& = {});
  void extract(const fs::path&, const ExtractOptions&, const Callbacks& = {});
  std::vector<fs::path> create(const CreateOptions&, const Callbacks& = {});
  void remove(const fs::path&, const std::vector<std::string>& entryIds, const std::string& password = {}, const Callbacks& = {});
  void rename(const fs::path&, const std::string& entryId, const std::string& newName, const std::string& password = {}, const Callbacks& = {});
  std::string identity() const;
 private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
}
