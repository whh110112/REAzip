#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace reazip {
namespace fs = std::filesystem;
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
struct Listing { std::string revision, format; std::uint64_t physicalSize{}; std::vector<Entry> entries; };
struct Callbacks {
  std::function<bool()> cancelled;
  std::function<void(std::uint64_t,std::uint64_t)> progress;
  std::function<std::string(bool)> password;
  std::function<Overwrite(const fs::path&)> overwrite;
  std::function<bool(const fs::path&)> trash;
  std::function<void(bool)> commitState;
};
struct ExtractOptions { fs::path destination; std::vector<std::string> entryIds; Overwrite overwrite{Overwrite::Ask}; std::string password,codepage; };
struct CreateOptions { fs::path output; std::vector<fs::path> inputs; std::string format{"zip"},password; int level{5}; bool aes{true},encryptHeaders{},deleteSources{}; std::uint64_t volumeSize{}; };
}
