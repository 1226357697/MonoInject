#pragma once
#include <stdint.h>


#pragma pack(push)
#pragma pack(1)
struct load_assembly_entry_t
{
  char* data;
  uint32_t data_len;
  char* name_pace;
  char* class_name;
  char* method_name;
};

struct load_assembly_t
{
  uint32_t count;
  char* mono_name;
  load_assembly_entry_t entrys[0];
};
#pragma pack(pop)


