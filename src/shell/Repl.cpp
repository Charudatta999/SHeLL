#include "shell/Repl.hpp"

#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/CaptureOutput.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "io/FdOps.hpp"
#include "parser/Parser.hpp"
#include "parser/ParserException.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellState.hpp"
#include "signals/Sigchld.hpp"
#include "signals/SignalManager.hpp"

#include <csignal>
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
    // Job control only when we're an interactive shell on a real
    // terminal. A piped/redirected stdin (scripts, tests) must not
    // ignore stop signals or grab terminal control — doing so would
    // SIGTTOU-stop the process the moment it writes output.
    if (isatty(STDIN_FILENO))
    {
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        m_state_->GetSignalMgr()->SetupInteractiveSignals();
        m_state_->EnableJobControl(true);
    }

    std::string line;
    std::string buffer; // accumulates lines of one incomplete command
    while (m_state_->IsRunning())
    {
        if (buffer.empty())
        {
            auto jobEvents = m_state_->GetJobs()->Reap();
            for (auto& event : jobEvents)
            {
                std::string word =
                    (event.state == JobTable::State::Stopped)
                        ? "Stopped"
                        : "Done";
                std::string out = "[" + std::to_string(event.id) +
                                  "]+ " + word + " " + event.command +
                                  "\n";
                io::fdops::WriteAll(STDOUT_FILENO, out);
            }
            PrintPrompt();
        }
        else
        {
            std::string out = "> "; // continuation
            io::fdops::WriteAll(STDOUT_FILENO, out);
        }
        if (!ReadLine(line))
        {
            if (!buffer.empty())
            {
                // Ctrl-D mid-continuation: drop the partial command,
                // report, and keep the shell alive.
                std::string out = "unexpected end of input \n";
                io::fdops::WriteAll(STDOUT_FILENO, out);
                buffer.clear();
                std::cin.clear();
                continue;
            }
            break;
        }
        if (!buffer.empty())
            buffer += '\n'; // newline is a command separator
        buffer += line;

        try
        {
            EvalLine(buffer);
            buffer.clear();
        }
        catch (const parser::IncompleteInputException&)
        {
            // valid so far, just ends too early -> keep reading
        }
    }
    return m_state_->GetShellExitCode();
}

void Repl::PrintPrompt()
{
    std::string out;

    std::string sigil = getuid() ? "$" : "#";
    out = "[" + m_state_->GetVar("USER").value_or("") + "@" +
          m_state_->GetCWD() + "]" + sigil;

    io::fdops::WriteAll(STDOUT_FILENO, out);
}

bool Repl::ReadLine(std::string& line)
{
    line.clear();
    io::fdops::ReadResult res = io::fdops::ReadResult::Ok;
    while (true)
    {
        char chr;
        res = io::fdops::ReadByte(STDIN_FILENO, chr);
        if (res == io::fdops::ReadResult::Interrupted)
        {
            if (signals::Sigchld::Consume())
            {
                auto jobEvents = m_state_->GetJobs()->Reap();
                for (auto& event : jobEvents)
                {
                    std::string word =
                        (event.state == JobTable::State::Stopped)
                            ? "Stopped"
                            : "Done";
                    std::string out = "[" + std::to_string(event.id) +
                                      "]+ " + word + " " +
                                      event.command + "\n";
                    io::fdops::WriteAll(STDOUT_FILENO, out);
                }
                PrintPrompt();
                continue;
            }
        }
        if (res == io::fdops::ReadResult::Ok)
        {
            if (chr == '\n')
                return true;
            line += chr;
        }
        if (res == io::fdops::ReadResult::Eof ||
            res == io::fdops::ReadResult::Error)
        {
            return false;
        }
    }
    return false;
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
        std::string out = std::string(ex.what()) + "\n";
        io::fdops::WriteAll(STDOUT_FILENO, out);
    }
    catch (const parser::IncompleteInputException&)
    {
        throw; // Run() keeps reading lines (PS2); not an error yet
    }
    catch (const parser::ParserException& ex)
    {
        m_state_->SetLastCommandExitCode(2);
        std::string out = std::string(ex.what()) + "\n";
        io::fdops::WriteAll(STDOUT_FILENO, out);
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        m_state_->SetLastCommandExitCode(1);
        std::string out = std::string(ex.what()) + "\n";
        io::fdops::WriteAll(STDOUT_FILENO, out);
    }
    return 1;
}

} // namespace shell
