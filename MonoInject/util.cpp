#include "util.h"
#include <Windows.h>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <fstream>

namespace fs = std::filesystem;

char* util::Utf16ToUtf8(const wchar_t* utf16)
{
  if (!utf16) return NULL;

  int size_needed = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
  if (size_needed <= 0) return NULL;


  char* utf8_str = (char*)malloc(size_needed);
  if (!utf8_str) return NULL;


  int result = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8_str, size_needed, NULL, NULL);
  if (result == 0) 
  {
    free(utf8_str);
    return NULL;
  }

  return utf8_str;
}

wchar_t* util::Utf8ToUtf16(const char* utf8)
{
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);

  wchar_t* utf16_str = (wchar_t*)malloc(size_needed * sizeof(wchar_t));
  if (!utf16_str) return NULL;

  int result = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16_str, size_needed);
  if (result == 0)
  {
    free(utf16_str);
    return NULL;
  }

  return utf16_str;
}

uint32_t util::hash(const void* key, size_t len, uint32_t seed)
{
  const uint8_t* data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  if (h1 == 0)
  {
    static uint32_t default_hash  = rand();
    h1 = default_hash;
  }

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  // body
  const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
  for (int i = -nblocks; i; i++) {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> (32 - 15));
    k1 *= c2;

    h1 ^= k1;
    h1 = (h1 << 13) | (h1 >> (32 - 13));
    h1 = h1 * 5 + 0xe6546b64;
  }

  // tail
  const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
  uint32_t k1 = 0;
  switch (len & 3) {
  case 3: k1 ^= tail[2] << 16;
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> (32 - 15));
    k1 *= c2;
    h1 ^= k1;
  }

  // finalization
  h1 ^= len;
  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;

  return h1;
}

std::string util::random_string(size_t length)
{
  const std::string charset =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;  // 真随机数种子
  std::mt19937 generator(rd());  // Mersenne Twister 引擎
  std::uniform_int_distribution<> distribution(0, charset.size() - 1);

  std::string result;
  result.reserve(length);  // 预分配内存，提升性能

  for (std::size_t i = 0; i < length; ++i) {
    result += charset[distribution(generator)];
  }

  return result;
}

std::string util::get_temp_dir()
{
#ifdef _WIN32
  const char* tmp = std::getenv("TEMP");
  if (!tmp) tmp = std::getenv("TMP");
#else
  const char* tmp = std::getenv("TMPDIR");
  if (!tmp) tmp = "/tmp";
#endif
  return std::string(tmp);
}

bool util::fast_write_file(const std::string& path, const void* data, size_t size)
{
  bool ret = false;
  std::ofstream ofs(path, std::ios::binary | std::ios::out);
  if (!ofs.is_open())
  {
    return false;
  }

  ofs.write((char*)data, size);
  ret = ofs.good();
  ofs.close();


  return ret;
}

bool util::write_temp_file(const void* data, size_t size, std::string* out_tmp_path)
{
  bool ret = false;
  std::string temp_dir = get_temp_dir();
  fs::path tmp_path = temp_dir / fs::path(random_string(7));

  while (fs::exists(tmp_path))
  {
    tmp_path = temp_dir / fs::path(random_string(7));
  }
  ret = fast_write_file(tmp_path.string(), data, size);

  if (ret && out_tmp_path)
  {
    *out_tmp_path = tmp_path.string();
  }

  return ret;
}
