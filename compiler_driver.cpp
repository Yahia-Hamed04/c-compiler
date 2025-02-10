#include <iostream>
using std::string;

int main(int argc, char* argv[]) {
 string filename, src;
 for (int i = 0; argv[argc - 1][i] != 0; i++) {
  if (argv[argc - 1][i] == '.') filename = src;

  src += argv[argc - 1][i];
 }

 int exit;
 string cmd = "gcc -E -P " + src + " -o pre.i";
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);

 cmd = "~/Documents/c-compiler/build/compiler pre.i asm.s ";
 if (argc > 2) cmd += argv[1];
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);
 if ((exit = system("rm pre.i")))  return WEXITSTATUS(exit);

 if (argc > 2) return 0;

 cmd = "gcc asm.s -o " + filename + " && rm asm.s";
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);
 
 return 0;
}