#include "helpers.h"

void error(std::string err_msg) {
 std::cerr << err_msg << std::endl;

 exit(1);
}

void error_at_line(Token token, std::string err_msg) {
 std::cerr << "Error at line " << token.line << ": " << err_msg
           << " Got " << token.to_string() << std::endl;

 exit(1);
}

void error_at_line(int line, std::string err_msg) {
 std::cerr << "Error at line " << line << ": " << err_msg << std::endl;

 exit(1);
}