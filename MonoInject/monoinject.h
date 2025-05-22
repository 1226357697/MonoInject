#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <vector>

#include "platform.h"
#include "datapool.h"
#include "util.h"

namespace fs = std::filesystem;
using namespace platform;

class MonoInject
{
public:

  int run(int argc, const char** argv);

private:
  std::optional<std::vector<uint8_t>> read_binrary_file(const std::string& path);
  bool get_mono_module(int pid, EnumModuleInfo& mono_module);
  bool parser_entry_method_name(const std::string& entry_full_name);
  bool make_load_assembly_data(DataPool& dataPool);
  bool inject_loader_dll();
  bool inject_dll(int pid, const std::string& dllPath);
  std::string get_template_loader_dll();
  bool check_argment(const std::string& gameName, const std::string& assemblyPath, const std::string& entryMethodFull);
private:

  EnumProcessInfo gameProcessInfo_;
  EnumModuleInfo monoModuleInfo_;
  std::string assembly_path_;
  std::string name_space_;
  std::string class_name_;
  std::string method_name_;
};

