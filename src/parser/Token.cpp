#include "parser/Token.hpp"

namespace parser
{

std::string_view tokenTypeName(TokenType type)
{
    switch (type)
    {
        // Words
        case TokenType::Word:             return "Word";
        case TokenType::SingleQuoted:     return "SingleQuoted";
        case TokenType::DoubleQuoted:     return "DoubleQuoted";

        // Pipe
        case TokenType::Pipe:             return "Pipe";
        case TokenType::PipeBoth:         return "PipeBoth";

        // Logical
        case TokenType::And:              return "And";
        case TokenType::Or:               return "Or";

        // Separators
        case TokenType::Semi:             return "Semi";
        case TokenType::DoubleSemi:       return "DoubleSemi";
        case TokenType::SemiAmp:          return "SemiAmp";
        case TokenType::DoubleSemiAmp:    return "DoubleSemiAmp";
        case TokenType::Background:       return "Background";
        case TokenType::Newline:          return "Newline";

        // Redirects: input
        case TokenType::RedirIn:          return "RedirIn";
        case TokenType::RedirReadWrite:   return "RedirReadWrite";
        case TokenType::HereDoc:          return "HereDoc";
        case TokenType::HereDocDash:      return "HereDocDash";
        case TokenType::HereString:       return "HereString";
        case TokenType::DupIn:            return "DupIn";

        // Redirects: output
        case TokenType::RedirOut:         return "RedirOut";
        case TokenType::RedirAppend:      return "RedirAppend";
        case TokenType::RedirClobber:     return "RedirClobber";
        case TokenType::DupOut:           return "DupOut";
        case TokenType::RedirBoth:        return "RedirBoth";
        case TokenType::RedirBothAppend:  return "RedirBothAppend";

        // Grouping
        case TokenType::LParen:           return "LParen";
        case TokenType::RParen:           return "RParen";
        case TokenType::LBrace:           return "LBrace";
        case TokenType::RBrace:           return "RBrace";
        case TokenType::DLParen:          return "DLParen";
        case TokenType::DRParen:          return "DRParen";
        case TokenType::DLBracket:        return "DLBracket";
        case TokenType::DRBracket:        return "DRBracket";

        // Keywords
        case TokenType::If:               return "If";
        case TokenType::Then:             return "Then";
        case TokenType::Elif:             return "Elif";
        case TokenType::Else:             return "Else";
        case TokenType::Fi:               return "Fi";
        case TokenType::While:            return "While";
        case TokenType::Until:            return "Until";
        case TokenType::Do:               return "Do";
        case TokenType::Done:             return "Done";
        case TokenType::For:              return "For";
         case TokenType::Foreach:         return "Foreach";
        case TokenType::End:              return "End";
        case TokenType::In:               return "In";
        case TokenType::Case:             return "Case";
        case TokenType::Esac:             return "Esac";
        case TokenType::Select:           return "Select";
        case TokenType::Function:         return "Function";
        case TokenType::Time:             return "Time";
        case TokenType::Bang:             return "Bang";

        // Structural
        case TokenType::Eof:              return "Eof";
    }
    return "Unknown";
}

// The literal source symbol for an operator/keyword token (so a command
// can be reconstructed for display). Word/quoted tokens carry their text
// in `value` and are handled by tokenText().
namespace
{
std::string_view tokenSymbol(TokenType type)
{
    switch (type)
    {
        case TokenType::Pipe:            return "|";
        case TokenType::PipeBoth:        return "|&";
        case TokenType::And:             return "&&";
        case TokenType::Or:              return "||";
        case TokenType::Semi:            return ";";
        case TokenType::DoubleSemi:      return ";;";
        case TokenType::SemiAmp:         return ";&";
        case TokenType::DoubleSemiAmp:   return ";;&";
        case TokenType::Background:      return "&";
        case TokenType::RedirIn:         return "<";
        case TokenType::RedirReadWrite:  return "<>";
        case TokenType::HereDoc:         return "<<";
        case TokenType::HereDocDash:     return "<<-";
        case TokenType::HereString:      return "<<<";
        case TokenType::DupIn:           return "<&";
        case TokenType::RedirOut:        return ">";
        case TokenType::RedirAppend:     return ">>";
        case TokenType::RedirClobber:    return ">|";
        case TokenType::DupOut:          return ">&";
        case TokenType::RedirBoth:       return "&>";
        case TokenType::RedirBothAppend: return "&>>";
        case TokenType::LParen:          return "(";
        case TokenType::RParen:          return ")";
        case TokenType::LBrace:          return "{";
        case TokenType::RBrace:          return "}";
        case TokenType::DLParen:         return "((";
        case TokenType::DRParen:         return "))";
        case TokenType::DLBracket:       return "[[";
        case TokenType::DRBracket:       return "]]";
        case TokenType::Bang:            return "!";
        default:                         return "";
    }
}
} // namespace

// One token's source form for display reconstruction: words as-is,
// quoted words re-wrapped, operators/keywords as their literal symbol
// or keyword text.
std::string tokenText(const Token& token)
{
    switch (token.type)
    {
        case TokenType::Word:         return token.value;
        case TokenType::SingleQuoted: return "'" + token.value + "'";
        case TokenType::DoubleQuoted: return "\"" + token.value + "\"";
        default:
        {
            std::string_view sym = tokenSymbol(token.type);
            if (!sym.empty())
                return std::string(sym);
            // Keywords (if/then/while/...) carry their text in value.
            return token.value;
        }
    }
}

} // namespace parser
