#pragma once
#include <cstdint>
#include <filesystem>
#ifdef __MINGW32__
#include <ghc/filesystem.hpp>
#endif
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace reazip {
#ifdef __MINGW32__
// GCC 13's Windows std::filesystem mis-parses the native \\?\ namespace.
// Use the same pinned filesystem adapter already bundled by bit7z.
namespace fs = ghc::filesystem;
#else
namespace fs = std::filesystem;
#endif
enum class ErrorCode { PasswordRequired, WrongPassword, UnsupportedFormat, UnsupportedOperation,
  EngineMissing, EngineArchitectureMismatch, PermissionDenied, CorruptArchive, UnsafeEntry, Cancelled, IoError };
class Error : public std::runtime_error {
 public: ErrorCode code; explicit Error(ErrorCode c, const std::string& detail) : std::runtime_error(detail), code(c) {}
};
enum class Overwrite { Ask, Overwrite, Skip, Rename };
struct Entry {
  std::uint32_t index{};
  std::string id, path, encryption{"unknown"};
  bool directory{}, encrypted{}, link{};
  std::uint64_t size{};
  std::optional<std::uint64_t> packed;
  std::optional<std::int64_t> modified;
};
struct Listing { std::string revision, sourceRevision, format; std::uint64_t physicalSize{}; std::vector<Entry> entries; };
struct Callbacks {
  std::function<bool()> cancelled;
  std::function<void(std::uint64_t,std::uint64_t)> progress;
  std::function<std::string(bool)> password;
  std::function<Overwrite(const fs::path&)> overwrite;
  std::function<bool(const fs::path&)> trash;
  std::function<void(bool)> commitState;
  std::function<void(const fs::path&)> outputFileBeforeCommit;
  std::function<void(std::size_t)> outputCommitted;
};
struct ExtractOptions { fs::path destination; std::vector<std::string> entryIds; Overwrite overwrite{Overwrite::Ask}; std::string password,codepage; };
struct CreateOptions { fs::path output; std::vector<fs::path> inputs; std::string format{"zip"},password; int level{5}; bool aes{true},encryptHeaders{},deleteSources{}; std::uint64_t volumeSize{}; };
}
