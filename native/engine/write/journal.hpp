#pragma once
#include <zenzip/types.hpp>
#include <memory>
namespace zenzip {
void syncFile(const fs::path&);
void commitFile(const fs::path& source,const fs::path& destination,bool replace=true);
void recoverTransactions(const fs::path& archive,const Callbacks&);
class OutputJournal {
 public:
  OutputJournal(fs::path stage,fs::path primary,std::vector<fs::path> files,std::vector<fs::path> outputs,std::vector<bool> replace);
  ~OutputJournal();
  void commit(const Callbacks&);bool rollback();
 private:struct Impl;std::unique_ptr<Impl> impl;
};
}
