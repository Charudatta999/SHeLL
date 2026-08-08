#include "shell/Repl.hpp"

#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/CaptureOutput.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "exec/ProcessSubstitution.hpp"
#include "io/FdOps.hpp"
#include "parser/Parser.hpp"
#include "parser/ParserException.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellState.hpp"
#include "signals/Sigchld.hpp"
#include "signals/SignalManager.hpp"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <sys/wait.h>
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
    , m_history_(m_state_->GetVar("HOME").value_or("") + "/.shell_history")
{
    m_cmdRunner_ = [this](const std::string& text) -> std::string
    {
        auto tokenizer = parser::Tokenizer(text);
        const auto& tokens = tokenizer.Tokenize();
        auto ast = parser::Parser(tokens).Parse();
        return exec::CaptureOutput(ast,
                                   m_state_,
                                   m_dispatcher_,
                                   m_cmdRunner_,
                                   m_procSubRunner_);
    };
    m_procSubRunner_ =
        exec::MakeProcSubRunner(m_state_, m_dispatcher_, m_cmdRunner_);
    m_executor_ = std::make_unique<exec::Executor>(m_state_,
                                                   m_dispatcher_,
                                                   m_cmdRunner_,
                                                   STDOUT_FILENO,
                                                   m_procSubRunner_);
    m_editor_ = std::make_unique<line::LineEditor>(m_terminal_, m_history_);
    m_history_.Load();
}

Repl::~Repl()
{
    // SuspendedCoro frames hold CompoundScope refs into *m_executor_.
    // m_executor_ is destroyed before m_state_ (declaration order), so
    // tearing jobs down with ShellState would UAF those frames. Clear
    // while the Executor is still alive.
    if (m_state_)
        m_state_->GetJobs()->Clear();
}

void Repl::SetArg0(std::string name)
{
    m_state_->SetArg0(std::move(name));
}

int Repl::Run()
{
    m_interactive_ = isatty(STDIN_FILENO);

    if (m_interactive_)
    {
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        m_state_->GetSignalMgr()->SetupInteractiveSignals();
        m_state_->EnableJobControl(true);
        m_state_->SaveTerminalModes();
        m_terminal_.EnableRawMode();
    }

    std::string buffer;
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
                                  "\r\n";
                io::fdops::WriteAll(STDOUT_FILENO, out);
            }
        }

        std::string line;
        if (m_interactive_)
        {
            auto prompt = buffer.empty() ? BuildPrompt() : std::string("> ");
            auto result = m_editor_->ReadLine(prompt);
            if (!result.has_value())
            {
                if (!buffer.empty())
                {
                    io::fdops::WriteAll(STDOUT_FILENO,
                                        "unexpected end of input\r\n");
                    buffer.clear();
                    continue;
                }
                break;
            }
            line = result.value();
        }
        else
        {
            if (buffer.empty())
                io::fdops::WriteAll(STDOUT_FILENO, BuildPrompt());
            else
                io::fdops::WriteAll(STDOUT_FILENO, "> ");

            if (!ReadLine(line))
            {
                if (!buffer.empty())
                {
                    io::fdops::WriteAll(STDOUT_FILENO,
                                        "unexpected end of input\n");
                    buffer.clear();
                    continue;
                }
                break;
            }
        }

        if (!buffer.empty())
            buffer += '\n';
        buffer += line;

        try
        {
            m_terminal_.DisableRawMode();
            EvalLine(buffer);
            if (m_interactive_ && !buffer.empty())
                m_history_.Add(buffer);
            buffer.clear();
            if (m_interactive_)
                m_terminal_.EnableRawMode();
        }
        catch (const parser::IncompleteInputException&)
        {
            if (m_interactive_)
                m_terminal_.EnableRawMode();
        }
    }

    m_terminal_.DisableRawMode();
    return m_state_->GetShellExitCode();
}

std::string Repl::BuildPrompt()
{
    std::string sigil = getuid() ? "$" : "#";
    return "[" + m_state_->GetVar("USER").value_or("") + "@" +
           m_state_->GetCWD() + "]" + sigil;
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
                io::fdops::WriteAll(STDOUT_FILENO, BuildPrompt());
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
        auto res = m_executor_->Run(parser);
        m_state_->SetLastCommandExitCode(res);
        CleanupProcSubs();
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
    CleanupProcSubs();
    return 1;
}

void Repl::CleanupProcSubs()
{
    // Closing the parent-side fd is what delivers EOF/SIGPIPE to a
    // process substitution's child once the foreground command no
    // longer needs it; do that before reaping. A child that hasn't
    // exited yet is polled again on the next command instead of
    // blocking the prompt (bash does not wait on these either).
    for (const auto& sub : m_state_->TakeProcSubs())
    {
        close(sub.fd);
        if (waitpid(sub.pid, nullptr, WNOHANG) == 0)
            m_pendingProcSubPids_.push_back(sub.pid);
    }
    std::erase_if(m_pendingProcSubPids_,
                  [](pid_t pid)
                  { return waitpid(pid, nullptr, WNOHANG) != 0; });
}

} // namespace shell
