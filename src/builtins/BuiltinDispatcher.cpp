#include "builtins/BuiltinDispatcher.hpp"

#include "builtins/BuiltInFunction.hpp"
#include "utils/ErrorCodes.hpp"

#include <memory>
#include <string>
#include <vector>

namespace builtins
{

BuiltinDispatcher::BuiltinDispatcher()
{
    m_table_["cd"]   = Cd;
    m_table_["echo"] = Echo;
    m_table_["exit"] = Exit;
    m_table_["pwd"]  = Pwd;
    m_table_["jobs"]  = Jobs;
    m_table_["fg"]  = Fg;
    m_table_["bg"]  = Bg;
    m_table_["wait"]  = Wait;
    m_table_["break"]  = Break;
    m_table_["continue"]  = Continue;
    m_table_["return"]  = Return;
    m_table_["export"]  = Export;
    m_table_["unset"]  = Unset;
    m_table_["set"]  = Set;
    m_table_["readonly"]  = Readonly;
    m_table_["suspend"]  = Suspend;

    m_descriptions_["cd"]   = "Change the current directory";
    m_descriptions_["echo"] = "Write arguments to standard output";
    m_descriptions_["exit"] = "Exit the shell";
    m_descriptions_["pwd"]  = "Print the current directory";
    m_descriptions_["jobs"] = "List active jobs";
    m_descriptions_["fg"]   = "Resume a job in the foreground";
    m_descriptions_["bg"]   = "Resume a job in the background";
    m_descriptions_["wait"] = "Wait for jobs to finish";
    m_descriptions_["break"]    = "Exit from a loop";
    m_descriptions_["continue"] = "Resume the next iteration of a loop";
    m_descriptions_["return"]   = "Return from a function or sourced script";
    m_descriptions_["export"]   = "Mark names for export to child processes";
    m_descriptions_["unset"]    = "Remove variable or function definitions";
    m_descriptions_["set"]      = "Set shell options and positional parameters";
    m_descriptions_["readonly"] = "Mark names as unmodifiable";
    m_descriptions_["suspend"]  = "Suspend the shell until SIGCONT";
}

std::vector<std::string> BuiltinDispatcher::Names() const
{
    std::vector<std::string> names;
    names.reserve(m_table_.size());
    for (const auto& entry : m_table_)
        names.push_back(entry.first);
    return names;
}

std::string BuiltinDispatcher::Description(const std::string& name) const
{
    auto itr = m_descriptions_.find(name);
    return itr != m_descriptions_.end() ? itr->second : std::string{};
}

bool BuiltinDispatcher::IsBuiltin(const std::string& name) const
{
    return m_table_.contains(name);
}

int BuiltinDispatcher::Run(const std::vector<std::string>& argv,
                           std::unique_ptr<BuiltinContext>& ctx)
{
    auto itr = m_table_.find(argv[0]);
    if (itr != m_table_.end())
    {
        return itr->second(argv, ctx);
    }
    return VALUE_NOT_FOUND;
}
} // namespace builtins