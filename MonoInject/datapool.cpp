#include "datapool.h"
#include "util.h"

DataEntry::DataEntry(uint32_t h, size_t p, size_t s)
  :hash(h),pos(p),size(s)
{
}

void DataEntry::addReloc(RelocType type, const std::string& name, size_t pos)
{
  uint32_t hash = util::hash(name.c_str(), name.size());
  relocs.emplace_back(hash, type, pos);
}


DataPool::DataPool()
{
}

void DataPool::clean()
{
  pool_.clear();
  entryMaps_.clear();
}

DataEntry* DataPool::addString(const std::string& name, const std::string& str)
{
  return addData(name, str.c_str(), str.size() + 1);
}

DataEntry* DataPool::addData(const std::string& name, const void* data, size_t size)
{
  uint32_t hash = util::hash(name.c_str(), name.size());

  size_t curPos = pool_.size();
  pool_.resize(pool_.size() + size);
  memcpy(pool_.data() + curPos, data, size);

  auto pair = entryMaps_.insert({ hash , DataEntry{hash, curPos, size}});
  return &pair.first->second;
}

const DataEntry* DataPool::findEntry(const std::string& name)
{
  uint32_t hash = util::hash(name.c_str(), name.size());
  return findEntry(hash);
}

const DataEntry* DataPool::findEntry(uint32_t hash)
{
  auto iter = entryMaps_.find(hash);
  if (iter != entryMaps_.end())
  {
    return &iter->second;
  }

  return NULL;
}

void* DataPool::findData(const std::string& name, size_t& size)
{
  auto entry = findEntry(name);
  if (!entry)
  {
    return NULL;
  }
  size = entry->size;

  return pool_.data() + entry->pos;
}

void DataPool::fixup(uint64_t base)
{
  for (auto&[k, v] : entryMaps_)
  {
    for (const auto& r : v.relocs)
    {
      if (r.type == REL_TYPE_SYMBOL)
      {
        uint64_t reloc_pos = v.pos + r.pos;

        const DataEntry* e = findEntry(r.hash);
        if (e)
        {
          *(uint64_t*)(pool_.data() + reloc_pos) = e->pos + base;
        }

      }
    }
  }
}

uint64_t DataPool::size()
{
  return pool_.size();
}

uint8_t* DataPool::data()
{
    return pool_.data();
}

