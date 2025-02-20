#include <iostream>
#include <queue>
using std::string;

int main(int argc, char* argv[]) {
 string filename, src, stage = "";
 std::queue<string> args;
 bool dont_link = false;

 for (int i = 1; i < argc; i++) {
  args.push(argv[i]);
 }

 for (string arg = args.front(); !args.empty(); args.pop(), arg = args.front()) {
  dont_link = dont_link || arg == "-c";

  if (arg.substr(0, 2) == "--") {
   stage = arg;
  } else if (arg != "-c") {
   for (int i = 0; i < arg.size(); i++) {
    if (arg[i] == '.') filename = src;
    
    src += arg[i];
   } 
  }
 }

 int exit;
 string cmd = "gcc -E -P " + src + " -o pre.i";
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);

 cmd = "~/Documents/c-compiler/build/compiler pre.i asm.s ";
 if (stage != "") cmd += stage;
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);
 if ((exit = system("rm pre.i")))  return WEXITSTATUS(exit);

 if (stage != "") return 0;

 cmd  = "gcc " + string(dont_link ? "-c " : "");
 cmd += "asm.s -o " + filename + string(dont_link ? ".o" : "") + " && rm asm.s";
 if ((exit = system(cmd.c_str()))) return WEXITSTATUS(exit);
 
 return 0;
}