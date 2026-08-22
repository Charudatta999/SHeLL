#include "shell/Repl.hpp"

int main(int argc, char* argv[])
{
    shell::Repl repl(shell::Repl::IsLoginInvocation(argc, argv));
    if (argc > 0 && argv[0] != nullptr)
        repl.SetArg0(argv[0]);
    return repl.Run();
}
