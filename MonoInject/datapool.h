#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

enum RelocType
{
  REL_TYPE_NONE = 0,
  REL_TYPE_SYMBOL,
};

struct Reloc
{
  uint32_t hash;
  RelocType type;
  size_t pos;
};

class DataPool;
struct DataEntry
{
  friend DataPool;
  uint32_t hash;
  size_t pos;
  size_t size;

  DataEntry(uint32_t h, size_t p, size_t s);

  void addReloc(RelocType type, const std::string& name, size_t pos);

private:
  std::vector<Reloc> relocs;
};

class DataPool
{
public:
  DataPool();

  void clean();

  DataEntry* addString(const std::string& name, const std::string& str);
  DataEntry* addData(const std::string& name, const void* data, size_t size);

  template<typename T>
  DataEntry* addObject(const std::string& name, const T& o);

  const DataEntry* findEntry(const std::string& name);
  const DataEntry* findEntry(uint32_t hash);
  void* findData(const std::string& name, size_t& size);

  void fixup(uint64_t base = 0);
  uint64_t size();
  uint8_t* data();

private:
  std::vector<uint8_t> pool_;
  std::map<uint32_t, DataEntry> entryMaps_;
};

template<typename T>
inline DataEntry* DataPool::addObject(const std::string& name, const T& o)
{
  return addData(name, &o, sizeof(T));
}
