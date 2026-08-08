#include "shell/Repl.hpp"

int main(int argc, char* argv[])
{
    shell::Repl repl;
    if (argc > 0 && argv[0] != nullptr)
        repl.SetArg0(argv[0]);
    return repl.Run();
}
