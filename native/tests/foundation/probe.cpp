#include <bit7z/bit7zlibrary.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <bit7z/bitarchivereader.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs = std::filesystem;
static bit7z::tstring native(const fs::path& p) { return p.native(); }
static void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int main(int argc, char** argv) {
  try {
    require(argc == 4, "usage: zenzip-probe LIBRARY TEST-DIRECTORY ISO-FIXTURE");
    const fs::path library = fs::absolute(fs::u8path(argv[1]));
    const fs::path work = fs::absolute(fs::u8path(argv[2]));
    fs::create_directories(work);
    bit7z::Bit7zLibrary engine(native(library));
    for (const auto& format : {std::make_pair("zip", &bit7z::BitFormat::Zip), std::make_pair("7z", &bit7z::BitFormat::SevenZip)}) {
      const auto source = work / fs::u8path(u8"测试 sample.txt");
      const std::string contents = u8"ZENzip 原生 library roundtrip\n";
      std::ofstream(source, std::ios::binary) << contents;
      const auto output = work / (std::string("sample.") + format.first);
      fs::remove(output);
      bit7z::BitFileCompressor creator(engine, *format.second);
      creator.compressFiles(std::vector<bit7z::tstring>{native(source)}, native(output));
      bit7z::BitArchiveReader reader(engine, native(output));
      require(reader.itemsCount() == 1, "roundtrip entry count");
      reader.test();
      const auto dest = work / (std::string("out-") + format.first);
      fs::create_directories(dest);
      reader.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
      reader.extractTo(native(dest));
      std::ifstream stream(dest / source.filename(), std::ios::binary);
      const std::string restored((std::istreambuf_iterator<char>(stream)), {});
      require(restored == contents, "roundtrip bytes differ");
      std::cout << format.first << ": create/list/test/extract PASS\n";
    }
    bit7z::BitArchiveReader iso(engine, native(fs::absolute(fs::u8path(argv[3]))));
    require(iso.itemsCount() == 1, "full engine ISO entry count");
    iso.test();
    std::cout << "ISO: full native engine format PASS\n";
    std::cout << "Engine 26.04; bit7z 4.1.0; native pointer bits " << sizeof(void*) * 8 << "; library " << library.u8string() << '\n';
    return 0;
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
