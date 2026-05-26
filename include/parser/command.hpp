#ifndef PARSER_COMMAND_HPP
#define PARSER_COMMAND_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace parser
{

// ─── Forward declarations ─────────────────────────────────────────────────────
struct AstNode;
using AstNodePtr = std::unique_ptr<AstNode>;

// ─── Base AST node ────────────────────────────────────────────────────────────
struct AstNode
{
    virtual ~AstNode() = default;
    int line = 0;
};

// ─── Redirect ─────────────────────────────────────────────────────────────────
// One redirect descriptor attached to a command.
// fd == -1 means "use the default fd for this kind"
//   (RedirIn→0, RedirOut→1, DupOut→1, etc.)
struct Redirect
{
    enum class Kind
    {
        In,          // <    stdin from file
        Out,         // >    stdout to file (truncate)
        Append,      // >>   stdout append
        Clobber,     // >|   force overwrite (bypass noclobber)
        DupIn,       // <&   duplicate input fd
        DupOut,      // >&   duplicate output fd
        Both,        // &>   stdout+stderr to file
        BothAppend,  // &>>  stdout+stderr append
        ReadWrite,   // <>   open for read+write
        HereDoc,     // <<   here-document (body stored in target)
        HereDocDash, // <<-  here-document, strip leading tabs
        HereString,  // <<<  here-string
    };

    Kind        kind;
    int         fd     = -1;  // explicit fd from input (e.g. 2 in "2>")
    std::string target;       // filename, dup target ("1" in 2>&1), or heredoc body
};

// ─── Simple command ───────────────────────────────────────────────────────────
// cmd arg1 arg2 VAR=x <in >out
struct SimpleCommand : AstNode
{
    std::vector<std::string>                        argv;
    std::vector<Redirect>                           redirects;
    std::vector<std::pair<std::string,std::string>> assignments; // VAR=val before cmd
};

// ─── Pipeline ─────────────────────────────────────────────────────────────────
// cmd1 | cmd2 | cmd3    or    ! cmd1 | cmd2
struct PipelineNode : AstNode
{
    std::vector<AstNodePtr> stages;
    bool                    bang = false; // ! prefix — negate exit status
};

// ─── Logical list ─────────────────────────────────────────────────────────────
// lhs && rhs    or    lhs || rhs
struct AndOrNode : AstNode
{
    enum class Op { And, Or };
    AstNodePtr lhs;
    Op         op;
    AstNodePtr rhs;
};

// ─── Statement list ───────────────────────────────────────────────────────────
// cmd1 ; cmd2 & cmd3
struct ListNode : AstNode
{
    struct Item
    {
        AstNodePtr node;
        bool       background; // true if followed by &
    };
    std::vector<Item> items;
};

// ─── Subshell ─────────────────────────────────────────────────────────────────
// ( list )
struct SubshellNode : AstNode
{
    AstNodePtr body;
};

// ─── Group command ────────────────────────────────────────────────────────────
// { list ; }
struct GroupNode : AstNode
{
    AstNodePtr body;
};

// ─── If statement ─────────────────────────────────────────────────────────────
// if cond; then body; elif cond; then body; else body; fi
struct IfNode : AstNode
{
    struct Branch
    {
        AstNodePtr condition;
        AstNodePtr body;
    };
    std::vector<Branch> branches;   // index 0 = if, 1..N = elif
    AstNodePtr          else_body;  // null if no else
};

// ─── While / Until ────────────────────────────────────────────────────────────
struct WhileNode : AstNode
{
    bool       until = false;
    AstNodePtr condition;
    AstNodePtr body;
};

// ─── For loop ─────────────────────────────────────────────────────────────────
// for var in word...; do body; done
struct ForNode : AstNode
{
    std::string              var;
    std::vector<std::string> words;  // empty = iterate over "$@"
    AstNodePtr               body;
};

// ─── Case statement ───────────────────────────────────────────────────────────
struct CaseNode : AstNode
{
    struct Arm
    {
        std::vector<std::string> patterns;
        AstNodePtr               body;
    };
    std::string      word;
    std::vector<Arm> arms;
};

// ─── Function definition ──────────────────────────────────────────────────────
// name() compound-command
struct FunctionNode : AstNode
{
    std::string name;
    AstNodePtr  body;  // always a compound command
};

} // namespace parser

#endif // PARSER_COMMAND_HPP
