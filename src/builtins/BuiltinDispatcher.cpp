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
}

bool BuiltinDispatcher::IsBuiltin(const std::string& name) const
{
    return m_table_.count(name);
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