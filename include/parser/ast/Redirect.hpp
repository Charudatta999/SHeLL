#ifndef PARSER_AST_REDIRECT_HPP
#define PARSER_AST_REDIRECT_HPP

#include "parser/Parser.hpp"
#include "parser/Token.hpp"
#include <string>
namespace parser::ast
{

struct Redirect
{
    enum class Kind
    {
        In,          // <    stdin from file
        Out,         // >    stdout to file (truncate)
        Append,      // >>   stdout append
        Clobber,     // >|   force overwrite
        DupIn,       // <&   duplicate input fd
        DupOut,      // >&   duplicate output fd
        Both,        // &>   stdout+stderr to file
        BothAppend,  // &>>  stdout+stderr append
        ReadWrite,   // <>   open read+write
        HereDoc,     // <<   here-document
        HereDocDash, // <<-  here-document, strip tabs
        HereString,  // <<<  here-string
    };

    Kind kind;
    int fd = -1;        // explicit fd (the 2 in "2>"); -1 = default for this kind
    std::string target; // filename, dup target, or heredoc body
};

} // namespace parser::ast
#endif // PARSER_AST_REDIRECT_HPP