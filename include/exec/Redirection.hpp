#ifndef EXEC_REDIRECTION_HPP
#define EXEC_REDIRECTION_HPP

#include "parser/ast/Redirect.hpp"

namespace exec
{
    bool ApplyRedirect(const parser::ast::Redirect& redirect);
} // namespace exec
#endif // EXEC_REDIRECTION_HPP