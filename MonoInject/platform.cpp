#include "platform.h"

#include "util.h"
#include <Windows.h>
#include <tlhelp32.h>

bool platform::enumerate_process(cb_enum_process_t cb)
{
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return false;

  bool ok = false;
  PROCESSENTRY32W tEntry = { 0 };
  tEntry.dwSize = sizeof(PROCESSENTRY32W);

  // Iterate threads
  for (BOOL success = Process32FirstW(snap, &tEntry);
    success != FALSE;
    success = Process32NextW(snap, &tEntry))
  {
    EnumProcessInfo info;
    info.pid = tEntry.th32ProcessID;
    info.ppid = tEntry.th32ParentProcessID;
    char* utf8_name = util::Utf16ToUtf8(tEntry.szExeFile);
    info.name = utf8_name;
    if (cb(info))
    {
      free(utf8_name);
      ok = true;
      break;
    }
    free(utf8_name);
  }

  CloseHandle(snap);
  return ok;
}

bool platform::enumerate_modules(int pid, cb_enum_module_t cb)
{
  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
  if (hSnap == INVALID_HANDLE_VALUE)
    return false;

  bool ok = false;
  MODULEENTRY32W me32;
  me32.dwSize = sizeof(MODULEENTRY32W);

  for (BOOL success = Module32FirstW(hSnap, &me32);
    success != FALSE;
    success = Module32NextW(hSnap, &me32))
  {
    EnumModuleInfo info;
    info.base = (uint64_t)me32.modBaseAddr;
    info.size = me32.modBaseSize;
    char* utf8_name = util::Utf16ToUtf8(me32.szModule);
    char* utf8_path = util::Utf16ToUtf8(me32.szExePath);
    info.name = utf8_name;
    info.path = utf8_path;
    if (cb(info))
    {
      free(utf8_name);
      free(utf8_path);
      ok = true;
      break;
    }
    free(utf8_name);
    free(utf8_path);
  }

  CloseHandle(hSnap);
  return ok;
}

void* platform::process_open(int pid)
{
    return (void*)OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

void platform::process_close(void* process)
{
  CloseHandle((HANDLE)process);
}

void* platform::thread_create(void* process, void* start_address, void* param, int* tid)
{
  return (void*)CreateRemoteThread(process, NULL,0, (LPTHREAD_START_ROUTINE)start_address, param, 0, (LPDWORD)tid);
}

void platform::thread_close(void* thread)
{
  CloseHandle((HANDLE)thread);
}

bool platform::wait_object(void* object, uint32_t ms)
{
  return WaitForSingleObject((HANDLE)object, ms) == WAIT_OBJECT_0;
}

uint64_t platform::memory_read(void* process, uint64_t address,  void* buffer, size_t size)
{
  uint64_t read_bytes = 0;
  ReadProcessMemory((HANDLE)process, (void*)address, buffer, size, &read_bytes);
  return read_bytes;
}

uint64_t platform::memory_write(void* process, uint64_t address, const void* buffer, size_t size)
{
  uint64_t write_bytes = 0;
  WriteProcessMemory((HANDLE)process, (void*)address, buffer, size, &write_bytes);
  return write_bytes;
}

uint64_t platform::memory_alloc(void* process, size_t size, uint64_t address)
{
  return (uint64_t)VirtualAllocEx((HANDLE)process, (void*)address, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void platform::memory_free(void* process, uint64_t address)
{
  VirtualFreeEx((HANDLE)process, (void*)address, 0, MEM_RELEASE);
}

void* platform::moudle_get_base(const char* name)
{
    return (void*)GetModuleHandleA(name);
}

void* platform::moudle_get_symbol(void* module, const char* name)
{
    return (void*)GetProcAddress((HMODULE)module, name);
}
