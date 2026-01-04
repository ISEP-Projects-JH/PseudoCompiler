#pragma once
#include <string>
#include <vector>
#include <stdexcept>

enum class TokenType
{
    If [[maybe_unused]],
    Else [[maybe_unused]],
    While [[maybe_unused]],
    Int,
    StringKw,
    Print,
    Prints [[maybe_unused]],
    Assign [[maybe_unused]],
    Comparison,
    Arth,
    L1 [[maybe_unused]],
    R1 [[maybe_unused]],
    L2 [[maybe_unused]],
    R2 [[maybe_unused]],
    Semicolon [[maybe_unused]],
    Separator [[maybe_unused]],
    Var,
    IntLit,
    String,
    End
};

struct Token final
{
    TokenType type{TokenType::End};
    std::string value;
    int line{0};
};

class [[maybe_unused]] TokenArray final
{
public:
    void push(const Token &t) { tokens.push_back(t); }

    [[maybe_unused]] [[nodiscard]] const Token &current() const
    {
        if (pos >= tokens.size())
            throw std::out_of_range("token pos");
        return tokens[pos];
    }

    [[maybe_unused]] void next()
    {
        if (pos + 1 < tokens.size())
            ++pos;
    }
    [[nodiscard]] bool empty() const { return tokens.empty(); }

    [[maybe_unused]] void appendEndIfMissing()
    {
        if (tokens.empty() || tokens.back().type != TokenType::End)
            tokens.push_back(Token{TokenType::End, "END", tokens.empty() ? 1 : tokens.back().line});
    }
    std::vector<Token> tokens{};
    size_t pos{};
};
