#pragma once
#include <string>


namespace util
{
  char* Utf16ToUtf8(const wchar_t* wstr);

  wchar_t* Utf8ToUtf16(const char* utf8);

  uint32_t hash(const void* key, size_t len, uint32_t seed = 0);

  std::string random_string(size_t length);

  std::string get_temp_dir();

  bool fast_write_file(const std::string& path, const void* data, size_t size);

  bool write_temp_file(const void* data, size_t size, std::string* out_tmp_path = nullptr);
};


