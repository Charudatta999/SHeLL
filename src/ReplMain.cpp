#include "shell/Repl.hpp"

int main(int argc, char* argv[])
{
    return shell::Repl(shell::Repl::IsLoginInvocation(argc, argv))
        .Run();
}