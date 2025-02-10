#include <iostream>
#include <fstream>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "tacky/tacky.h"
#include "code_gen/code_gen.h"
#include "emitter.h"
#include "helpers.h"

string readFile(const char* filePath) {
 std::ifstream ifs(filePath, std::ios::in | std::ios::binary | std::ios::ate);

 std::ifstream::pos_type fileSize = ifs.tellg();
 ifs.seekg(0, std::ios::beg);
 std::vector<char> bytes(fileSize);
 ifs.read(bytes.data(), fileSize);
 return string(bytes.data(), fileSize);
}

int main(int argc, char* argv[]) {
 int mode = 100;

 if (argc > 3 && argv[3][0] == '-') {
  string flag = argv[3];

  if (flag == "--lex") {
   mode = 1;
  } else if (flag == "--parse") {
   mode = 2;
  } else if (flag == "--validate") {
   mode = 3;
  } else if (flag == "--tacky") {
   mode = 4;
  } else if (flag == "--codegen") {
   mode = 5;
  }
 }

 string src = readFile(argv[1]);

 Lexer lexer(src);
 if (mode == 1) return 0;
 Parser::CParser parser(lexer, mode >= 3);
 if (mode == 2) return 0;
 TACKYifier tackyifier(parser);
 if (mode == 4) return 0;
 Generator gen(tackyifier);
 if (mode == 5) return 0;
 Emitter emitter(gen);

 std::ofstream prog;
 if (argc > 1) {
  prog.open(argv[2]);
 } else prog.open("out.s");

 prog << emitter.get_code();

 prog.close();

 return 0;
}