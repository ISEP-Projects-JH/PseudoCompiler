#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace pseu::lexer {

    enum class TokenType {
        If,
        Else [[maybe_unused]],
        While [[maybe_unused]],
        Int [[maybe_unused]],
        StringKw [[maybe_unused]],
        Print,
        Prints [[maybe_unused]],
        Assign [[maybe_unused]],
        Comparison [[maybe_unused]],
        Arth [[maybe_unused]],
        L1 [[maybe_unused]],
        R1 [[maybe_unused]],
        L2 [[maybe_unused]],
        R2 [[maybe_unused]],
        Semicolon [[maybe_unused]],
        Separator [[maybe_unused]],
        Var [[maybe_unused]],
        IntLit [[maybe_unused]],
        String,
        End
    };

    struct Token final {
        TokenType type{TokenType::End};
        std::string value;
        int line{0};
    };

} // namespace pseu::lexer
