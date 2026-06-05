#include "exec/Redirection.hpp"

#include "parser/ast/Redirect.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

namespace
{
// dup `opened` onto the redirect's fd (explicit, or `defaultFd` if -1), then close.
void Redirect(int opened, int explicitFd, int defaultFd)
{
    int tgt = (explicitFd == -1) ? defaultFd : explicitFd;
    dup2(opened, tgt);
    close(opened);
}
} // namespace

namespace exec
{
bool ApplyRedirect(const parser::ast::Redirect& redirect)
{
    using kind = parser::ast::Redirect::Kind;
    switch (redirect.kind)
    {
        case kind::In:
        {
            int opened = open(redirect.target.c_str(), O_RDONLY);
            if (opened < 0)
                return false;
            Redirect(opened, redirect.fd, 0);
            break;
        }
        case kind::Out:
        {
            int opened = open(redirect.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (opened < 0)
                return false;
            Redirect(opened, redirect.fd, 1);
            break;
        }
        case kind::Clobber:
        {
            int opened = open(redirect.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (opened < 0)
                return false;
            Redirect(opened, redirect.fd, 1);
            break;
        }
        case kind::Append:
        {
            int opened = open(redirect.target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (opened < 0)
                return false;
            Redirect(opened, redirect.fd, 1);
            break;
        }
        case kind::Both:
        {
            int opened = open(redirect.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (opened < 0)
                return false;
            dup2(opened, 1);
            dup2(opened, 2);
            close(opened);
            break;
        }
        case kind::BothAppend:
        {
            int opened = open(redirect.target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (opened < 0)
                return false;
            dup2(opened, 1);
            dup2(opened, 2);
            close(opened);
            break;
        }
        case kind::DupOut:
        {
            int src = atoi(redirect.target.c_str());
            int tgt = (redirect.fd == -1) ? 1 : redirect.fd;
            dup2(src, tgt);
            break;
        }
        case kind::DupIn:
        {
            int src = atoi(redirect.target.c_str());
            int tgt = (redirect.fd == -1) ? 0 : redirect.fd;
            dup2(src, tgt);
            break;
        }
        case kind::HereDoc:
        case kind::HereDocDash:
        case kind::HereString:
        {
            int p[2];
            if (pipe(p) < 0)
                return false;
            write(p[1], redirect.target.data(), redirect.target.size());
            close(p[1]);
            dup2(p[0], 0);
            close(p[0]);
            break;
        }
        default:
        {
            return false;
        }
    };
    return true;
}
} // namespace exec