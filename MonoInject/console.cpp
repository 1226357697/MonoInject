#include "console.h"

#include <iostream>
#include <cstdarg>

#include<windows.h>

void console::initalize(const std::string& name)
{
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  SetConsoleTitleA(name.c_str());
}

void console::writeLine(const std::string& msg)
{
  std::cout<< msg <<std::endl;
}

void console::writeLine(const char* fmt, ...)
{
  std::va_list ap;
  va_start(ap, fmt);
  std::vprintf(fmt, ap);

  std::cout << std::endl;
  va_end(ap);
}

