#ifndef EXEC_CONTROLFLOW_HPP
#define EXEC_CONTROLFLOW_HPP

namespace exec
{

struct LoopControl
{
    enum class Kind
    {
        Break,
        Continue,
    };

    Kind kind;
    int level;
};

struct FunctionReturn
{
    int status;
};

} // namespace exec
#endif // EXEC_CONTROLFLOW_HPP
