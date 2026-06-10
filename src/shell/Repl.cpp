#include "shell/Repl.hpp"

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "parser/Parser.hpp"
#include "parser/ParserException.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/ShellState.hpp"
#include "arithmetic/ArithmeticException.hpp"

#include <cstddef>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <unistd.h>

extern char** environ;

namespace
{
    std::map<std::string,std::string> LoadEnvironMap()
    {
        std::map<std::string, std::string> vars;
        for (char** envPtr = environ; *envPtr != nullptr; ++envPtr)
        {
            std::string entry = *envPtr;
            auto pos = entry.find('=');
            if (pos == std::string::npos) continue;
            auto key   = entry.substr(0, pos);
            auto value = entry.substr(pos + 1);
            vars.emplace(key,value);
        }
        return vars;
    }
}
namespace shell
{

Repl::Repl()
    : m_state_(std::make_unique<ShellState>(LoadEnvironMap()))
    , m_dispatcher_(std::make_unique<builtins::BuiltinDispatcher>())
{
}

Repl::~Repl() = default;

int Repl::Run()
{

    std::string line;
    while (m_state_->IsRunning())
    {
        PrintPrompt();
        if (!ReadLine(line))
            break;
        EvalLine(line);
    }
    return m_state_->GetShellExitCode();
}

void Repl::PrintPrompt()
{
   std::cout << "$" << std::flush;
}

bool Repl::ReadLine(std::string& line)
{
    return static_cast<bool>(std::getline(std::cin, line));
}

int Repl::EvalLine(const std::string& line)
{
    try
    {
        auto tokenizer = parser::Tokenizer(line);

        const auto& tokens = tokenizer.Tokenize();
        const auto& parser = parser::Parser(tokens).Parse();
        auto executor = exec::Executor(m_state_, m_dispatcher_);
        auto res = executor.Run(parser);
        m_lastStatus_ = res;
        return res;
    }
    catch (const exec::ExecException& ex)
    {
        std::cout << ex.what() << "\n";
    }
    catch (const parser::ParserException& ex)
    {
        std::cout << ex.what() << "\n";
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        std::cout << ex.what() << "\n";
    }
    return 1;
}

} // namespace shell
