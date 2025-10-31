#pragma once
#include <iostream>
#include <cstring>
#include <cstdint>
#include "lexer/tokens.h"

// helper type for the std::visit
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void error(std::string err_msg);
void error_at_line(int line, std::string err_msg);
void error_at_line(Token token, std::string err_msg);
size_t truncate(size_t num);