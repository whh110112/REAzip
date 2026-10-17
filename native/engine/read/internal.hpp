#pragma once
#include <reazip/engine.hpp>
#include <bit7z/bit7zlibrary.hpp>
#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitarchiveitemoffset.hpp>
#include <bit7z/biterror.hpp>
namespace reazip {
inline bit7z::tstring ns(const fs::path& p) { return p.native(); }
inline bit7z::tstring ns(const std::string& p) { return fs::u8path(p).native(); }
inline std::string utf8(const bit7z::tstring& s) { return fs::path(s).u8string(); }
struct Engine::Impl {
  fs::path path;
  bit7z::Bit7zLibrary library;
  explicit Impl(const fs::path& p) : path(p),library(ns(p)) {}
};
void checkCancelled(const Callbacks&);
void attach(bit7z::BitAbstractArchiveHandler&, const Callbacks&);
[[noreturn]] void translate(const bit7z::BitException&, const std::string& password, const Callbacks&);
std::string revision(const fs::path&);
std::string formatName(const bit7z::BitInFormat&);
std::unique_ptr<bit7z::BitArchiveReader> openReader(bit7z::Bit7zLibrary&,const fs::path&,const std::string&,const std::string&,const Callbacks&);
}
