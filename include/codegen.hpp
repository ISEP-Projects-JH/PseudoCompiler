/**
 * @file codegen.hpp
 * @author JeongHan-Bae &lt;mastropseudo&#64;gmail.com&gt;
 * @brief NASM code generation backend for PseudoCompiler IR.
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

#include "ir.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pseu::codegen {

    /**
     * @brief NASM assembly generator from IR.
     *
     * This class consumes a linear IR instruction sequence and emits
     * x86-64 NASM-compatible assembly code.
     *
     * Responsibilities include:
     * <ul>
     *   <li>Lowering IR instructions to concrete assembly sequences</li>
     *   <li>Managing variable storage and temporaries</li>
     *   <li>Emitting helper routines (e.g., print functions) on demand</li>
     * </ul>
     *
     * The generator operates on pure-value IR instructions and performs
     * static dispatch via <code>std::visit</code>, without RTTI or virtual
     * dispatch.
     */
    class CodeGenerator final {
    public:
        /**
         * @brief Construct a code generator.
         *
         * @param arr         Linear IR instruction array.
         * @param identifiers Map of variable names to declared types.
         * @param constants   Map of string constants to literal values.
         * @param tempmap     Optional mapping for temporaries.
         */
        CodeGenerator(const ir::InterCodeArray &arr,
                      const std::unordered_map<std::string, std::string> &identifiers,
                      const std::unordered_map<std::string, std::string> &constants,
                      const std::unordered_map<std::string, std::string> &tempmap = {});
        /**
         * @brief Emit assembly output to a file.
         *
         * @param path Output file path.
         */
        void writeAsm(const std::string &path);

    private:
        /// @brief Append raw text to the output buffer.
        void pr(std::string_view s);

        /// @brief Emit global/static variable definitions.
        void gen_variables();

        /// @brief Emit program entry prologue.
        void gen_start();

        /// @brief Emit program epilogue.
        void gen_end();

        /// @brief Generate assembly for the full IR stream.
        void gen_code();

        /// @brief Lower an assignment instruction.
        void gen_assignment(const ir::AssignmentCode &a);

        /// @brief Lower an unconditional jump instruction.
        void gen_jump(const ir::JumpCode &j);

        /// @brief Lower a label definition.
        void gen_label(const ir::LabelCode &l);

        /// @brief Lower a conditional comparison instruction.
        void gen_compare(const ir::CompareCodeIR &c);

        /// @brief Lower a print instruction.
        void gen_print(const ir::PrintCodeIR &p);

        /// @brief Emit helper routine for integer printing.
        void gen_print_num_function();

        /// @brief Emit helper routine for string printing.
        void gen_print_string_function();

        /**
         * @brief Resolve a variable or temporary into a concrete symbol.
         *
         * @param a       Variable name.
         * @param tempmap Temporary substitution map.
         * @return Resolved symbol name.
         */
        static std::string handleVar(const std::string &a, const std::unordered_map<std::string, std::string> &tempmap);

    private:
        const ir::InterCodeArray &arr;
        std::unordered_map<std::string, std::string> ids;
        std::unordered_map<std::string, std::string> consts;
        std::unordered_map<std::string, std::string> tempmap;
        std::vector<char> out;
        bool need_print_num = false;
        bool need_print_string = false;
    };

} // namespace pseu::codegen
