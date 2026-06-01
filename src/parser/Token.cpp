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

} // namespace parser
