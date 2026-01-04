/**
 * @file tokens.hpp
 * @author JeongHan-Bae &lt;mastropseudo&#64;gmail.com&gt;
 * @brief Lexical token definitions for the PseudoCompiler frontend.
 *
 * @license MIT
 *
 * @verbatim
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * @endverbatim
 */

#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace pseu::lexer {

    /**
     * @brief Enumeration of all lexical token kinds.
     *
     * TokenType represents the complete set of terminals
     * recognized by the lexer and consumed by the parser.
     * The set is closed and not intended for extension.
     */
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

    /**
     * @brief Lexical token structure.
     *
     * Token is a simple value type carrying the token kind,
     * its textual representation, and source line information.
     */
    struct Token final {
        TokenType type{TokenType::End};
        std::string value;
        int line{0};
    };

} // namespace pseu::lexer
