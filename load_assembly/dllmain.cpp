// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <stdarg.h>
#include <iostream>
#include "load_assembly.h"

load_assembly_t* g_load_assembly = NULL;

typedef enum {
  MONO_SECURITY_MODE_NONE,
  MONO_SECURITY_MODE_CORE_CLR,
} MonoSecurityMode;

typedef enum {
  MONO_IMAGE_OK,
  MONO_IMAGE_ERROR_ERRNO,
  MONO_IMAGE_MISSING_ASSEMBLYREF,
  MONO_IMAGE_IMAGE_INVALID
} MonoImageOpenStatus;

using MonoDomain = void;
using MonoThread = void;
using MonoAssembly = void;
using MonoImage = void;
using MonoClass = void;
using MonoMethod = void;
using MonoObject = void;

using mono_get_root_domain_t = MonoDomain*(*)(void);
using mono_thread_attach_t = MonoThread* (*)(MonoDomain* domain);
using mono_security_set_mode_t =  void (*)(MonoSecurityMode mode);
using mono_image_open_from_data_full_t = MonoImage* (*)(char* data, uint32_t data_len, int32_t need_copy, MonoImageOpenStatus* status, int32_t refonly);
using mono_assembly_load_from_full_t = MonoAssembly* (*)(MonoImage* image, const char* fname, MonoImageOpenStatus* status, int32_t refonly);
using mono_assembly_get_image_t = MonoImage* (*)(MonoAssembly* assembly);
using mono_class_from_name_t = MonoClass*(*)(MonoImage* image, const char* name_space, const char* name);
using mono_class_get_method_from_name_t = MonoMethod* (*)(MonoClass* klass, const char* name, int param_count);
using mono_runtime_invoke_t = MonoObject* (*)(MonoMethod* method, void* obj, void** params, MonoObject** exc);

struct mono_mehtod_desc_t
{
  void** pptr;
  const char* name;
};

static void msg(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  char fmt_buf[512];
  char* text = fmt_buf;

  int size = vsnprintf(fmt_buf, sizeof(fmt_buf) - 1, fmt, ap);
  if (size >= sizeof(fmt_buf) - 1)
  {
    text = (char*)malloc(size + 1);
    vsnprintf(text, size, fmt, ap);
    MessageBoxA(NULL, text, NULL, MB_OK);
    free(text);
  }
  else
  {
    MessageBoxA(NULL, text, NULL, MB_OK);
  }

  va_end(ap);
}

DWORD WINAPI WorkThreadRoutine(LPVOID parameter)
{
  DWORD exitCode = 0;
  HMODULE hModule = (HMODULE)parameter;
  HMODULE mono = NULL;
  mono_get_root_domain_t mono_get_root_domain = NULL;
  mono_thread_attach_t mono_thread_attach = NULL;
  mono_security_set_mode_t mono_security_set_mode = NULL;
  mono_image_open_from_data_full_t mono_image_open_from_data_full = NULL;
  mono_assembly_load_from_full_t mono_assembly_load_from_full = NULL;
  mono_assembly_get_image_t mono_assembly_get_image = NULL;
  mono_class_from_name_t mono_class_from_name = NULL;
  mono_class_get_method_from_name_t mono_class_get_method_from_name = NULL;
  mono_runtime_invoke_t mono_runtime_invoke = NULL;
  MonoDomain* rootDomain = NULL;
  MonoThread* rootThread = NULL;

  mono_mehtod_desc_t  mehtod_descs[] = {
  {(void**)&mono_get_root_domain, "mono_get_root_domain"},
  {(void**)&mono_thread_attach, "mono_thread_attach"},
  {(void**)&mono_security_set_mode, "mono_security_set_mode"},
  {(void**)&mono_image_open_from_data_full, "mono_image_open_from_data_full"},
  {(void**)&mono_assembly_load_from_full, "mono_assembly_load_from_full"},
  {(void**)&mono_assembly_get_image, "mono_assembly_get_image"},
  {(void**)&mono_class_from_name, "mono_class_from_name"},
  {(void**)&mono_class_get_method_from_name, "mono_class_get_method_from_name"},
  {(void**)&mono_runtime_invoke, "mono_runtime_invoke"},
  };

  // msg("[load_assembly] g_load_assembly:%p", g_load_assembly);
  // __debugbreak();

  load_assembly_t* load_assembly  = g_load_assembly;
  if (load_assembly == NULL || load_assembly->count == 0)
  {
    msg("[load_assembly] invalid argment");
    goto clean_up;
  }

  mono = GetModuleHandleA(load_assembly->mono_name);
  if (mono == NULL)
  {
    msg("[load_assembly] not found mono dll");
    goto clean_up;
  }

  for (int i =0 ;i < sizeof(mehtod_descs)/sizeof(mehtod_descs[0]); ++i)
  {
    mono_mehtod_desc_t& desc = mehtod_descs[i];
    *desc.pptr = (void*)GetProcAddress(mono, desc.name);

    if (*desc.pptr == NULL)
    {
      msg("[load_assembly] not function: %s", desc.name);
      goto clean_up;
    }
  }

  rootDomain = mono_get_root_domain();
  rootThread = mono_thread_attach(rootDomain);
  mono_security_set_mode(MONO_SECURITY_MODE_NONE);
  
  for (uint32_t i = 0; i < load_assembly->count; i++)
  {
    load_assembly_entry_t& entry = load_assembly->entrys[i];
    MonoImageOpenStatus openStatus;

    MonoImage* rawImage = mono_image_open_from_data_full(entry.data, entry.data_len, true, &openStatus, false);
    if (openStatus != MONO_IMAGE_OK)
    {
      msg("[load_assembly] failed to mono_image_open_from_data_full. status:%d data: %p ", openStatus, entry.data);
      goto clean_up;
    }

    MonoAssembly* assembly = mono_assembly_load_from_full(rawImage, "", &openStatus, false);
    //if (openStatus != MONO_IMAGE_OK)
    if (assembly == NULL)
    {
      msg("[load_assembly] failed to mono_assembly_load_from_full. status:%d data: %p ", openStatus, entry.data);
      goto clean_up;
    }

    MonoImage* image = mono_assembly_get_image(assembly);
    if (image == NULL)
    {
      msg("[load_assembly] failed tomono_assembly_get_image. data: %p ", openStatus, entry.data);
      goto clean_up;
    }

    MonoClass* klass = mono_class_from_name(image, entry.name_pace, entry.class_name);
    if (image == NULL)
    {
      msg("[load_assembly] failed mono_class_from_name. data: %p  class name:%s.%s", openStatus, entry.data, entry.name_pace, entry.class_name);
      goto clean_up;
    }

    MonoMethod* method =  mono_class_get_method_from_name(klass, entry.method_name, 0);
    if (image == NULL)
    {
      msg("[load_assembly] failed mono_class_get_method_from_name. data: %p  method name:%s.%s", openStatus, entry.data, entry.name_pace, entry.class_name, entry.method_name);
      goto clean_up;
    }

    mono_runtime_invoke(method, NULL,NULL, NULL);
  }


clean_up:
  FreeLibraryAndExitThread(hModule, exitCode);
  return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
      DisableThreadLibraryCalls(hModule);
      HANDLE work_thread = CreateThread(NULL, 0, WorkThreadRoutine, hModule, 0, NULL);
      if (work_thread == NULL)
      {
        return FALSE;
      }

      CloseHandle(work_thread);
      break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

