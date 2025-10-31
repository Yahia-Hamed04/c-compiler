#pragma once
#include <cstddef>
#include <cstring>
#include <string>

using std::string;
using std::size_t;

enum TokenType {
 // Keywords & Literals Start
 Identifier,
 Return,
 Number,
 Unsigned_Number,
 Long_Number,
 Unsigned_Long_Number,
 If,
 Else,
 Goto,
 Do,
 While,
 For,
 Break,
 Continue,
 Switch,
 Case,
 Default,
 Void,
  // Specifiers Begin
   // Types Begin
 Int,
 Long,
 Signed,
 Unsigned,
   // Types End
 Static,
 Extern,
  // Specifiers End
 // Keywords & Literals End
 // Punctuation Start
 Left_Curly,
 Right_Curly,
 Left_Paren,
 Right_Paren,
 Question,
 Colon,
 Semicolon,
 Comma,
 // Punctuation End
 // Operators Start
  // Unary Start
 Plus_Plus,
 Minus_Minus,
 Tilde,
 Exclamation,
  // Binary Start
 Plus,
 Minus,
  // Unary End
 Percent,
 Asterisk,
 Slash,
 Ampersand,
 Ampersand_Ampersand,
 Pipe,
 Pipe_Pipe,
 Caret,
 Less,
 Less_Equal,
 Less_Less,
 Greater,
 Greater_Equal,
 Greater_Greater,
 Exclamation_Equal,
 Equal_Equal,
 Equal,
 Plus_Equal,
 Minus_Equal,
 Asterisk_Equal,
 Slash_Equal,
 Percent_Equal,
 Ampersand_Equal,
 Pipe_Equal,
 Caret_Equal,
 Less_Less_Equal,
 Greater_Greater_Equal,
  // Binary End
 // Operators End
 TokenType_Count,
};

// https://en.cppreference.com/w/c/language/operator_precedence
static int precedence[TokenType_Count] = {
 [TokenType::Asterisk]              = 50,
 [TokenType::Slash]                 = 50,
 [TokenType::Percent]               = 50,
 [TokenType::Plus]                  = 45,
 [TokenType::Minus]                 = 45,
 [TokenType::Less_Less]             = 40,
 [TokenType::Greater_Greater]       = 40,
 [TokenType::Greater]               = 35,
 [TokenType::Greater_Equal]         = 35,
 [TokenType::Less]                  = 35,
 [TokenType::Less_Equal]            = 35,
 [TokenType::Exclamation_Equal]     = 30,
 [TokenType::Equal_Equal]           = 30,
 [TokenType::Ampersand]             = 25,
 [TokenType::Caret]                 = 24,
 [TokenType::Pipe]                  = 23,
 [TokenType::Ampersand_Ampersand]   = 10,
 [TokenType::Pipe_Pipe]             = 5,
 [TokenType::Question]              = 3,
 [TokenType::Equal]                 = 1,
 [TokenType::Plus_Equal]            = 1,
 [TokenType::Minus_Equal]           = 1,
 [TokenType::Asterisk_Equal]        = 1,
 [TokenType::Slash_Equal]           = 1,
 [TokenType::Percent_Equal]         = 1,
 [TokenType::Ampersand_Equal]       = 1,
 [TokenType::Pipe_Equal]            = 1,
 [TokenType::Caret_Equal]           = 1,
 [TokenType::Less_Less_Equal]       = 1,
 [TokenType::Greater_Greater_Equal] = 1,
};

static const char* strings[TokenType_Count] = {
 [TokenType::Identifier]            = "Identifier",
 [TokenType::Int]                   = "Int",
 [TokenType::Long]                  = "Long",
 [TokenType::If]                    = "If",
 [TokenType::Else]                  = "Else",
 [TokenType::Goto]                  = "Goto",
 [TokenType::Do]                    = "Do",
 [TokenType::While]                 = "While",
 [TokenType::For]                   = "For",
 [TokenType::Break]                 = "Break",
 [TokenType::Continue]              = "Continue",
 [TokenType::Switch]                = "Switch",
 [TokenType::Case]                  = "Case",
 [TokenType::Default]               = "Default",
 [TokenType::Static]                = "Static",
 [TokenType::Extern]                = "Extern",
 [TokenType::Plus]                  = "Plus",
 [TokenType::Plus_Plus]             = "Plus_Plus",
 [TokenType::Minus]                 = "Minus",
 [TokenType::Minus_Minus]           = "Minus_Minus",
 [TokenType::Percent]               = "Percent",
 [TokenType::Ampersand]             = "Ampersand",
 [TokenType::Ampersand_Ampersand]   = "Ampersand_Ampersand",
 [TokenType::Pipe]                  = "Pipe",
 [TokenType::Pipe_Pipe]             = "Pipe_Pipe",
 [TokenType::Caret]                 = "Caret",
 [TokenType::Less]                  = "Less",
 [TokenType::Less_Equal]            = "Less_Equal",
 [TokenType::Less_Less]             = "Less_Less",
 [TokenType::Greater]               = "Greater",
 [TokenType::Greater_Equal]         = "Greater_Equal",
 [TokenType::Greater_Greater]       = "Greater_Greater",
 [TokenType::Exclamation]           = "Exclamation",
 [TokenType::Exclamation_Equal]     = "Exclamation_Equal",
 [TokenType::Equal]                 = "Equal",
 [TokenType::Equal_Equal]           = "Equal_Equal",
 [TokenType::Plus_Equal]            = "Plus_Equal",
 [TokenType::Minus_Equal]           = "Minus_Equal",
 [TokenType::Asterisk_Equal]        = "Asterisk_Equal",
 [TokenType::Slash_Equal]           = "Slash_Equal",
 [TokenType::Percent_Equal]         = "Percent_Equal",
 [TokenType::Ampersand_Equal]       = "Ampersand_Equal",
 [TokenType::Pipe_Equal]            = "Pipe_Equal",
 [TokenType::Caret_Equal]           = "Caret_Equal",
 [TokenType::Less_Less_Equal]       = "Less_Less_Equal",
 [TokenType::Greater_Greater_Equal] = "Greater_Greater_Equal",
 [TokenType::Number]                = "Number",
 [TokenType::Left_Paren]            = "Left_Paren",
 [TokenType::Right_Paren]           = "Right_Paren",
 [TokenType::Left_Curly]            = "Left_Curly",
 [TokenType::Right_Curly]           = "Right_Curly",
 [TokenType::Return]                = "Return",
 [TokenType::Semicolon]             = "Semicolon",
 [TokenType::Asterisk]              = "Asterisk",
 [TokenType::Slash]                 = "Slash",
 [TokenType::Tilde]                 = "Tilde",
 [TokenType::Void]                  = "Void"
};

struct Token {
 TokenType type;
 char* start;
 size_t length, line;
 
 string to_string() {
  char *cstr = new char[length + 1];
  cstr[length] = 0;
  strncpy(cstr, start, length);
  
  return string(cstr);
 }
};