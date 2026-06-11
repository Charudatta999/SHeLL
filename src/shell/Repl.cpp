#include "shell/Repl.hpp"

#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/CaptureOutput.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "parser/Parser.hpp"
#include "parser/ParserException.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellState.hpp"

#include <cstddef>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <unistd.h>

extern char** environ;

namespace
{
std::map<std::string, std::string> LoadEnvironMap()
{
    std::map<std::string, std::string> vars;
    for (char** envPtr = environ; *envPtr != nullptr; ++envPtr)
    {
        std::string entry = *envPtr;
        auto pos = entry.find('=');
        if (pos == std::string::npos)
            continue;
        auto key = entry.substr(0, pos);
        auto value = entry.substr(pos + 1);
        vars.emplace(key, value);
    }
    return vars;
}
} // namespace

namespace shell
{

Repl::Repl()
    : m_state_(std::make_unique<ShellState>(LoadEnvironMap()))
    , m_dispatcher_(std::make_unique<builtins::BuiltinDispatcher>())
{
    m_cmdRunner_ = [this](const std::string& text) -> std::string
    {
        auto tokenizer = parser::Tokenizer(text);
        const auto& tokens = tokenizer.Tokenize();
        auto ast = parser::Parser(tokens).Parse();
        return exec::CaptureOutput(ast,
                                   m_state_,
                                   m_dispatcher_,
                                   m_cmdRunner_);
    };
}

Repl::~Repl() = default;

int Repl::Run()
{

    std::string line;
    while (m_state_->IsRunning())
    {
        auto finishedJobs = m_state_->GetJobs()->Reap();
        for (auto& job : finishedJobs)
        {
            std::cout << "[" << job.id << "]+ Done " << job.command
                      << "\n";
        }
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
        auto executor =
            exec::Executor(m_state_, m_dispatcher_, m_cmdRunner_);
        auto res = executor.Run(parser);
        m_state_->SetLastCommandExitCode(res);
        return res;
    }
    catch (const exec::ExecException& ex)
    {
        m_state_->SetLastCommandExitCode(1);
        std::cout << ex.what() << "\n";
    }
    catch (const parser::ParserException& ex)
    {
        m_state_->SetLastCommandExitCode(2);
        std::cout << ex.what() << "\n";
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        m_state_->SetLastCommandExitCode(1);
        std::cout << ex.what() << "\n";
    }
    return 1;
}

} // namespace shell
