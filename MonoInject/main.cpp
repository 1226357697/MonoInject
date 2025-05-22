#include <iostream>
#include "util.h"
#include "monoinject.h"


int wmain(int argc, wchar_t** argv)
{
  int result = 0;
  const char** args = new const  char*[argc];
  for (int i = 0; i < argc; ++i)
  {
    args[i] = util::Utf16ToUtf8(argv[i]);
  }

#if 0
  const char* tmp_args[] = {
    "self.exe",
    "--game_name",
    "EscapeFromTarkov.exe",
    "--assembly",
    "D:\\Work\\CSharp\\GameHackBeta\\GameHackBeta\\bin\\Debug\\GameHackBeta.dll",
    "--entry_method",
    "GameHackBeta.HackLoader.Load",
  };
  int tmp_argc = sizeof(tmp_args) / sizeof(tmp_args[0]);
  argc = tmp_argc;
  args = tmp_args;
#endif // 0



  result = MonoInject().run(argc, args);

  for (int i = 0; i < argc; ++i)
  {
    free(const_cast<char*>( args[i]));
  }

  delete args;

  return result;
}