
#include "shell/ShellCompleter.hpp"

namespace shell
{

ShellCompleter::ShellCompleter(
    std::unique_ptr<ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher) :
    m_state_(state)
    ,m_dispatcher_(dispatcher)
{
}

line::Result  ShellCompleter::Complete(const std::string& line,
                                      std::size_t cursor) const
{
    
}

} // namespace shell