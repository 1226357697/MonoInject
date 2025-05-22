#pragma once
#include <string>

namespace console
{
  void initalize(const std::string& name);

  void writeLine(const std::string& msg);

  void writeLine(const char* fmt, ...);
};

