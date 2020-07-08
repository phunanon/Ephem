#include "Parser.hpp"
#include "Enums.hpp"
#include <cstring>
#include <cstdlib>
#include <functional>
#include <queue>
#include <algorithm>
using namespace std;

bool isWhite (char c) {
  return c == ' ' || c == '\n';
}

bool isNumber (const string& s) {
  //return ('0' <= s[0] && s[0] <= '9')
  //  || (s[0] == '-' && '0' <= s[1] && s[1] <= '9');
  return strspn(s.c_str(), "-.0123456789bw") == s.size();
}


//Remove extraneous space such as
//  double spaces, spaces before or after parens,
//  single-line comments, multi-line comments
string removeExtraneousSpace (const string& input) {
  string output = "";
  char prev = ' ';
  bool inString = false;
  bool inSComment = false, inMComment = false;
  for (uint16_t i = 0, iLen = input.length(); i < iLen; ++i) {
    char c = input[i];
    char next = i+1 == iLen ? '\0' : input[i+1];
    bool inComment = inSComment || inMComment;
    //Handle entering/leaving string
    if (!inComment && c == '"' && prev != '\\')
      inString = !inString;
    //Handle comments
    if (!inString) {
      if (!inComment && c == '/') {
        inSComment = next == '/';
        inMComment = next == '*';
      }
      if ((inSComment && c == '\n')
       || (inMComment && c == '*' && next == '/')) {
        i += inMComment ? 2 : 1;
        inSComment = inMComment = false;
        continue;
      }
      if (inSComment || inMComment)
        continue;
    }
    //Handle extraneous spaces
    if (!inString && isWhite(c)) {
      bool doubleWhite       = isWhite(prev);
      bool whiteAfterOpen    = prev == '(' || prev == '[';
      bool whiteBeforeClose  = next == ')' || next == ']';
      if (doubleWhite || whiteAfterOpen || whiteBeforeClose)
        continue;
    }
    if (c == '\n') c = ' ';
    output += (prev = c);
  }
  return output;
}


struct Token {
  enum {
    Unknown,
    LParen, RParen, LSquare, RSquare,
    Hash, Period, Para,
    Char, Number, String, Symbol
  } type;
  string str;
};

vector<Token> tokenise (const string& input) {
  auto tokens = vector<Token>();
  for (uint16_t i = 0, iLen = input.length(); i < iLen; ++i) {
    char c = input[i];
    //Handle # ( ) [ ]
    {
      auto type = Token::Unknown;
      switch (c) {
        case '#': type = Token::Hash;    break;
        case '(': type = Token::LParen;  break;
        case ')': type = Token::RParen;  break;
        case '[': type = Token::LSquare; break;
        case ']': type = Token::RSquare; break;
      }
      if (type != Token::Unknown) {
        tokens.push_back(Token{type, string(1, c)});
        continue;
      }
    }
    //Handle string
    if (c == '"') {
      auto end = input.find('"', ++i);
      auto str = string(&input[i], &input[end]);
      tokens.push_back(Token{Token::String, str});
      i += str.length();
      continue;
    }
    //Skip spaces
    if (c == ' ' || c == '\n')
      continue;
    //Collect next string before
    //  space newline " ( ) [ ]
    auto nextDelim = input.find_first_of(" \n\"()[]", i);
    if (nextDelim == string::npos)
      nextDelim = input.length();
    string next = string(&input[i], &input[nextDelim]);
    //Handle character, number, parameter, period, or other symbol
    {
      auto type = Token::Symbol;
      if (c == '\\')
        type = Token::Char;
      else if (c == '%')
        type = Token::Para;
      else if (isNumber(next) && !(next.length() == 1 && (c == '-' || c == '+')))
        type = Token::Number;
      else if (c == '.')
        type = Token::Period;
      tokens.push_back(Token{type, next});
    }
    i += next.length() - 1;
  }
  return tokens;
}


//Separate all tokens by the highest level of parenthesis
vector<vector<Token>> separate (vector<Token> tokens) {
  auto funcs = vector<vector<Token>>();
  uint8_t depth = 0;
  for (auto t : tokens) {
    if (!depth) funcs.push_back(vector<Token>());
    funcs.back().push_back(t);
    if (t.type == Token::LParen || t.type == Token::LSquare) ++depth;
    if (t.type == Token::RParen || t.type == Token::RSquare) --depth;
  }
  return funcs;
}


Op symToOp (const char* symbol) {
  for (uint8_t o = 0; ops[o]; ++o)
    if (!strcmp(symbol, ops[o]))
      return (Op)o;
  return O_None;
}

//Take a vector of tokens and parameters,
//  which constitutes one form,
//  and return a root Cell
Cell* cellise (deque<Token> &tokens, vector<string> paras) {
  Cell* head = nullptr;
  Cell* prev = nullptr;
  Cell* cell = nullptr;
  while (tokens.size()) {
    auto token = tokens.front();
    tokens.pop_front();
    //Recursively generate Cell for a form
    if (token.type == Token::LParen || token.type == Token::Hash) {
      bool isLambda = token.type == Token::Hash;
      if (isLambda) tokens.pop_front();
      Cell* form = cellise(tokens, paras);
      cell = new Cell{Value(Data{.cell=form}, isLambda ? T_Lamb : T_Cell)};
    } else
    //... or return this form's head
    if (token.type == Token::RParen || token.type == Token::RSquare)
      return head;
    else
    //... or generate vector form for the following arguments
    if (token.type == Token::LSquare) {
      tokens.push_front(Token{Token::Symbol, "vec"});
      Cell* vecForm = cellise(tokens, paras);
      cell = new Cell{Value(Data{.cell=vecForm}, T_Cell)};
    } else {
    //... or generate Cell for this other type of argument
      Data data;
      Type type = T_N;
      switch (token.type) {
        case Token::Char: {
          //FIXME with nl sp
          data.s08 = token.str[1];
          type = T_S08;
          break;
        }
        case Token::Number: {
          //FIXME for bytes and words
          if (token.str.find('.') != string::npos) {
            if (token.str[0] == '.')
              token.str = '0' + token.str;
            data.d32 = stof(token.str);
            type = T_D32;
          } else {
            data.u32 = stoi(token.str);
            type = T_U32;
          }
          break;
        }
        case Token::String: {
          data.ptr = new string(token.str);
          type = T_Str;
          break;
        }
        case Token::Para: {
          token.str.erase(token.str.begin());
          data.u08 = token.str.length() ? stoi(token.str) : 0;
          type = T_Para;
          break;
        }
        case Token::Symbol: {
          //A symbol is  
          //  a bool, nil, variable, op, parameter, or function
          {
            char ch = token.str[0];
            //True/False
            if (ch == 'T' || ch == 'F') {
              data.tru = ch == 'T';
              type = T_Boo;
              break;
            }
            //Nil
            if (ch == 'N') break;
            //Variable
            if (ch == '$') {
              data.u32 = hash<string>{}(token.str);
              type = T_Var;
              break;
            }
          } { //Op
            Op op = symToOp(token.str.c_str());
            if (op) {
              data.op = op;
              type = T_Op;
              break;
            }
          } { //Param
            auto pIt = find(paras.begin(), paras.end(), token.str); 
            if (pIt != paras.end()) {
              data.u32 = pIt - paras.begin();
              type = T_Para;
              break;
            }
          } { //Func
            data.fID = hash<string>{}(token.str);
            type = T_Func;
          }
          break;
        }
      }
      cell = new Cell{Value(data, type)};
    }
    //Prepare previous with next,
    //  and retain head if haven't already
    if (prev) prev->next = cell;
    prev = cell;
    if (!head) head = cell;
  }
  return head;
}


//Take a vector of tokens,
//  either a function declaration or entry form,
//  and return a vector of cells which are forms
pair<fid, vector<Cell*>> cellise (vector<Token> form) {
  fid id = 0;
  auto paras = vector<string>();
  //Check if this is a function declaration
  //  or part of the entry function
  if (form.size() > 1 && form[1].str == "fn") {
    id = hash<string>{}(form[2].str);
    //Collect param symbols
    argnum t = 4;
    //TODO: destructuring goes here
    for (; form[t].type != Token::RSquare; ++t)
      paras.push_back(form[t].str);
    form = vector<Token>(&form[t+1], &form.back());
  }
  //Cellise all function forms, or the one entry form
  auto formsCells = vector<Cell*>();
  {
    auto formTokens = deque<Token>();
    uint8_t depth = 0;
    for (auto t : form) {
      formTokens.push_back(t);
      if (t.type == Token::LParen || t.type == Token::LSquare) ++depth;
      if (t.type == Token::RParen || t.type == Token::RSquare) --depth;
      if (!depth) {
        if (t.type == Token::LParen)
          formTokens.pop_front(); //Pop first paren
        formsCells.push_back(cellise(formTokens, paras));
      }
    }
  }
  return pair<fid, vector<Cell*>>(id, formsCells);
} 


map<fid, vector<Cell*>> Parser::parse (string source) {
  auto noExtraneousSpace = removeExtraneousSpace(source);
//printf("%s\n", noExtraneousSpace.c_str());
  auto tokens = tokenise(noExtraneousSpace);
//for (auto t : tokens) printf("%d %s\t", t.type, t.str.c_str());
//printf("\n");
  auto separatedTokens = separate(tokens);
  auto funcs = map<fid, vector<Cell*>>();
  for (auto tokens : separatedTokens) {
    auto cells = cellise(tokens);
    if (cells.first)
      funcs.insert(cells);
    else
      if (cells.second.size())
        funcs[0].push_back(cells.second[0]);
  }
  return funcs;
}