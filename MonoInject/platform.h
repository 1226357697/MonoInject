#pragma once
#include <string>
#include <functional>

namespace platform
{
  struct EnumProcessInfo
  {
    int pid;
    int ppid;
    std::string name;
  };

  struct EnumModuleInfo
  {
    uint64_t base;
    uint32_t size;
    std::string name;
    std::string path;
  };

  using cb_enum_process_t = std::function<bool(EnumProcessInfo&)>;
  using cb_enum_module_t = std::function<bool(EnumModuleInfo&)>;

  bool enumerate_process(cb_enum_process_t cb);

  bool enumerate_modules(int pid, cb_enum_module_t cb);
  
  void* process_open(int pid);

  void process_close(void* process);

  void* thread_create(void* process, void* start_address, void* param, int* tid = NULL);

  void thread_close(void* thread);

  bool wait_object(void* object, uint32_t ms = -1);

  uint64_t memory_read(void* process, uint64_t address, void* buffer, size_t size);

  uint64_t memory_write(void* process, uint64_t address, const void* buffer, size_t size);

  uint64_t memory_alloc(void* process, size_t size, uint64_t address = 0);

  void memory_free(void* process, uint64_t address);

  void* moudle_get_base(const char* name);

  void* moudle_get_symbol(void* module, const char* name);

};

