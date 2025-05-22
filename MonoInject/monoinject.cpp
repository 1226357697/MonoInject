#include "monoinject.h"

#include <stdarg.h>
#include <tuple>
#include <array>
#include <linux-pe/linuxpe>
#include <fstream>
#include <assert.h>

#include "console.h"
#include "cxxopts.hpp"

#include "../load_assembly/load_assembly.h"



bool MonoInject::get_mono_module(int pid, EnumModuleInfo& mono_module)
{
  std::array<const char*, 2> mono_module_names = {
    "mono.dll",
    "mono-2.0-bdwgc.dll",
  };

  return  enumerate_modules(pid, [this, &mono_module_names, &mono_module](EnumModuleInfo& info)
    {
      auto iter = std::find(std::begin(mono_module_names), std::end(mono_module_names), info.name);
      if (iter != std::end(mono_module_names))
      {
        std::optional<std::vector<uint8_t>> buffer = read_binrary_file(info.path);
        if (buffer.has_value())
        {
          win::image_x64_t* image = (win::image_x64_t*)buffer->data();
          win::data_directory_t* dir = image->get_directory(win::directory_entry_export);
          if (dir->present())
          {
            win::export_directory_t* export_dir = image->rva_to_ptr<win::export_directory_t>(dir->rva, dir->size);
            uint32_t* name_table = image->rva_to_ptr<uint32_t>(export_dir->rva_names);
            for (int i = 0; i < export_dir->num_names; ++i)
            {
              // 通过导出函数判断是不是mono引擎dll
              char* name = image->rva_to_ptr<char>( name_table[i]);
              if (strcmp(name, "mono_init_version") == 0)
              {
                mono_module = info;
                return true;
              }
            }
          }
        }
        
      }
      return false;
    });
}

std::optional<std::vector<uint8_t>> MonoInject::read_binrary_file(const std::string& path)
{
  std::vector<uint8_t> bytes;
  wchar_t* u16_path = util::Utf8ToUtf16(path.c_str());

  std::ifstream ifs(u16_path, std::ios::binary);
  free(u16_path);

  if (!ifs.is_open())
  {
    return std::nullopt;
  }
  ifs.seekg(0, std::ios::end);
  bytes.resize(ifs.tellg());
  ifs.seekg(0, std::ios::beg);
  ifs.read((char*)bytes.data(), bytes.size());
  ifs.close();
  return std::make_optional<std::vector<uint8_t>>( bytes);
}

bool
MonoInject::parser_entry_method_name(const std::string& entry_full_name)
{
  std::vector<std::string> result;
  size_t start = 0;
  size_t end;

  while ((end = entry_full_name.find('.', start)) != std::string::npos) {
    result.push_back(entry_full_name.substr(start, end - start));
    start = end + 1;
  }
  result.push_back(entry_full_name.substr(start)); // 最后一个字段

  if (result.size() != 3)
  {
    return false;
  }
  name_space_ = result.at(0);
  class_name_ = result.at(1);
  method_name_ = result.at(2);

  return true;
}

bool MonoInject::make_load_assembly_data(DataPool& dataPool)
{
  std::optional<std::vector<uint8_t>> assembly_raw_bytes = read_binrary_file(assembly_path_);
  if (!assembly_raw_bytes.has_value())
  {
    console::writeLine("Failed to read %s", assembly_path_.c_str());
    return false;
  }

  dataPool.addData("assembly_data", assembly_raw_bytes->data(), assembly_raw_bytes->size());
  dataPool.addString("name_space", name_space_);
  dataPool.addString("class_name", class_name_);
  dataPool.addString("method_name", method_name_);

  load_assembly_entry_t entry;
  entry.data_len = assembly_raw_bytes->size();

  load_assembly_t load_assembly;
  load_assembly.count = 1;

  dataPool.addString("mono_name", monoModuleInfo_.name);
  DataEntry* load_assembly_data = dataPool.addObject("load_assembly", load_assembly);
  DataEntry* entry_data = dataPool.addObject("entry", entry);

  load_assembly_data->addReloc(REL_TYPE_SYMBOL, "mono_name", offsetof(load_assembly_t, mono_name));

  entry_data->addReloc(REL_TYPE_SYMBOL, "assembly_data", offsetof(load_assembly_entry_t, data));
  entry_data->addReloc(REL_TYPE_SYMBOL, "name_space", offsetof(load_assembly_entry_t, name_pace));
  entry_data->addReloc(REL_TYPE_SYMBOL, "class_name", offsetof(load_assembly_entry_t, class_name));
  entry_data->addReloc(REL_TYPE_SYMBOL, "method_name", offsetof(load_assembly_entry_t, method_name));

  return true;
}

bool MonoInject::inject_loader_dll()
{
  DataPool dataPool;
  if (!make_load_assembly_data(dataPool))
  {
    return false;
  }

  void* process = process_open(gameProcessInfo_.pid);
  uint64_t param = memory_alloc(process, dataPool.size());

  dataPool.fixup(param);
  memory_write(process, param, dataPool.data(), dataPool.size());
  const DataEntry* load_assembly_data = dataPool.findEntry("load_assembly");
  assert(load_assembly_data);

  std::optional<std::vector<uint8_t>> buffer = read_binrary_file(get_template_loader_dll());
  if(!buffer)
    return false;

  win::image_t<true>* image = (win::image_t<true>*)buffer->data();
  int g_load_assembly_order = 1;

  win::data_directory_t* dir = image->get_directory(win::directory_entry_export);
  if (!dir->present())
  {
    return false;
  }

  win::export_directory_t* export_dir = image->rva_to_ptr<win::export_directory_t>(dir->rva, dir->size);
  if (export_dir->num_functions != 1)
  {
    return false;
  }

  uint32_t* export_function = image->rva_to_ptr<uint32_t>(export_dir->rva_functions);
  uint32_t rva = export_function[g_load_assembly_order - export_dir->base];

  uint32_t g_load_assembly_offset = image->rva_to_fo(rva);
  *(uint64_t*)(buffer->data() +  g_load_assembly_offset) = param + load_assembly_data->pos;

  std::string tmp_path = "1.dll";
  //if (!util::write_temp_file(buffer->data(), buffer->size(), &tmp_path))
  //{
  //  return false;
  //}
  util::fast_write_file(tmp_path, buffer->data(), buffer->size());

  if (!inject_dll(gameProcessInfo_.pid, fs::absolute(tmp_path).string() ))
  {
    return false;
  }

  return true;
}

bool MonoInject::inject_dll(int pid, const std::string& dllPath)
{

  void* LoadLibraryA = moudle_get_symbol(moudle_get_base("kernel32.dll"), "LoadLibraryA");
  if (!LoadLibraryA)
  {
    console::writeLine("not found symbol: LoadLibraryA");
    return false;
  }

  bool ret = false;
  void* process = NULL;
  void* thread = NULL;
  uint64_t dllpath_ptr = 0;

  process = process_open(pid);
  if (!process)
  {
    console::writeLine("fialed to open the process:%d", pid);
    goto clean_up;
  }

  dllpath_ptr = memory_alloc(process, dllPath.size() + 1);
  if (dllpath_ptr == 0)
  {
    console::writeLine("fialed to alloc mmeory for process:%d", pid);
    goto clean_up;
  }

  memory_write(process, dllpath_ptr, dllPath.c_str(), dllPath.size() + 1);

  thread = thread_create(process, LoadLibraryA, (void*)dllpath_ptr);
  if (thread == NULL)
  {
    console::writeLine("fialed to create thread for process:%d", pid);
    goto clean_up;
  }

  if (!wait_object(thread))
  {
    console::writeLine("The target thread terminated with an exception");
    goto clean_up;
  }
  ret = true;

clean_up:

  if(dllpath_ptr)
    memory_free(process, dllpath_ptr);

  if (thread)
    process_close(thread);

  if(process)
    process_close(process);

  return ret;
}


std::string MonoInject::get_template_loader_dll()
{
  return "load_assembly.dll";
}

bool MonoInject::check_argment(const std::string& gameName, const std::string& assemblyPath, const std::string& entryMethodFull)
{ 
  bool ret = false;
  // check argment
  ret = enumerate_process([&gameName, this](EnumProcessInfo& info)->bool
    {
      if (info.name.compare(gameName) == 0)
      {
        gameProcessInfo_ = info;
        return true;
      }
      return false;
    });

  if (!ret)
  {
    console::writeLine("not found %s", gameName.c_str());
    return false;
  }

  ret = get_mono_module(gameProcessInfo_.pid, monoModuleInfo_);
  if (!ret)
  {
    console::writeLine("%s is not a mono process", gameName.c_str());
    return false;
  }

  if (!fs::exists(assemblyPath))
  {
    console::writeLine("The assembly %s does not exist", assemblyPath.c_str());
    return false;
  }

  if (!parser_entry_method_name(entryMethodFull))
  {
    console::writeLine("invalid entry_method: %s", entryMethodFull.c_str());
    return false;
  }
  assembly_path_ = assemblyPath;
  return true;
}

int MonoInject::run(int argc, const char** argv)
{
  console::initalize("MonoInject");
  cxxopts::Options options("MonoInject", "Inject the specified Unity Mono assembly into the game process");
  std::optional<std::string> gameName;
  std::optional<std::string> assemblyPath;
  std::optional<std::string> entryMethod;
  bool ret = 0;

  options.add_options()
    ("game_name", "Game process name", cxxopts::value<std::string>())
    ("assembly", "Unity mono assembly", cxxopts::value<std::string>())
    ("entry_method", "Execute the full path of the entry function (namespace.class.method)", cxxopts::value<std::string>())
    ;

  auto result = options.parse(argc, argv);

  gameName = result["game_name"].as_optional<std::string>();
  assemblyPath = result["assembly"].as_optional<std::string>();
  entryMethod = result["entry_method"].as_optional<std::string>();

  if (!gameName.has_value())
  {
    console::writeLine("Unspecified game process name");
    console::writeLine(options.help());
    return 1;
  }

  if (!assemblyPath.has_value())
  {
    console::writeLine("Unspecified unity mono assembly");
    console::writeLine(options.help());
    return 1;
  }

  if (!entryMethod.has_value())
  {
    console::writeLine("Unspecified execute the full path of the entry function");
    console::writeLine(options.help());
    return 1;
  }

  if (!check_argment(gameName.value(), assemblyPath.value(), entryMethod.value()))
  {
    return 1;
  }

  if (!inject_loader_dll())
  {
    return 1;
  }

  console::writeLine("Injection successful");
  return 0;
}
