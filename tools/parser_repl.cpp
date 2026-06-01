// =========================================================
// parser_repl — manual test harness for the parser layer.
//
// Reads a line, tokenizes -> parses -> prints the AST tree.
// Lets you eyeball that the parser builds the right structure.
//
//   echo 'make && ls | grep foo' | ./build/bin/parser_repl
//   ./build/bin/parser_repl   (interactive)
// =========================================================

#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "parser/ast/AstPrinter.hpp"

#include <iostream>
#include <string>

int main()
{
    std::string line;
    std::cout << "parser> ";
    while (std::getline(std::cin, line))
    {
        if (!line.empty())
        {
            try
            {
                parser::Tokenizer tokenizer(line);
                parser::Parser    parser(tokenizer.Tokenize());
                auto              tree = parser.Parse();

                parser::ast::AstPrinter printer;
                tree->Accept(printer);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "error: " << ex.what() << "\n";
            }
        }
        std::cout << "parser> ";
    }
    std::cout << "\n";
    return 0;
}
