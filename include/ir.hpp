/**
 * @file ir.hpp
 * @author JeongHan-Bae &lt;mastropseudo&#64;gmail.com&gt;
 * @brief Intermediate Representation (IR) definitions and generation utilities.
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
#include <unordered_map>
#include <memory>
#include <variant>
#include "ast.hpp"
#include <jh/meta>
#include <jh/pool>

namespace pseu::ir {

    struct AssignmentCode;
    struct JumpCode;
    struct LabelCode;
    struct CompareCodeIR;
    struct PrintCodeIR;

    /**
     * @brief Mix helper for 64-bit hash composition.
     *
     * This uses a simple xor–multiply scheme compatible with FNV-style hashing.
     */
    constexpr inline std::uint64_t hash_mix(std::uint64_t h,
                                            std::uint64_t v) noexcept {
        return (h ^ v) * 1099511628211ull;
    }

    /**
     * @brief Variant covering all IR instruction forms.
     *
     * IRInstr is a pure value type and contains no behavior.
     */
    using IRInstr = std::variant<
            AssignmentCode,
            JumpCode,
            LabelCode,
            CompareCodeIR,
            PrintCodeIR
    >;

    /**
     * @brief Primary template for node hashing.
     *
     * Specializations are provided for each IR instruction type.
     */
    template<typename T>
    struct node_hash;

    /**
     * @brief Assignment IR instruction.
     *
     * Represents:
     * <pre>
     *   var = left [op right]
     * </pre>
     */
    struct AssignmentCode final {
        std::string var;
        std::string left;
        std::string op;    // empty if none
        std::string right; // may be empty

        bool operator==(const AssignmentCode &) const = default;
    };

    /// @brief Hash specialization for AssignmentCode.
    template<>
    struct node_hash<AssignmentCode> {
        std::uint64_t operator()(AssignmentCode const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.var.data(), a.var.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.left.data(), a.left.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.op.data(), a.op.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.right.data(), a.right.size()));

            return h;
        }
    };

    /**
     * @brief Unconditional jump instruction.
     */
    struct JumpCode final {
        std::string dist;

        bool operator==(const JumpCode &) const = default;
    };

    /// @brief Hash specialization for JumpCode.
    template<>
    struct node_hash<JumpCode> {
        std::uint64_t operator()(JumpCode const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.dist.data(), a.dist.size()));

            return h;
        }
    };

    /**
     * @brief Label definition instruction.
     */
    struct LabelCode final {
        std::string label;

        bool operator==(const LabelCode &) const = default;
    };

    /// @brief Hash specialization for LabelCode.
    template<>
    struct node_hash<LabelCode> {
        std::uint64_t operator()(LabelCode const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.label.data(), a.label.size()));

            return h;
        }
    };

    /**
     * @brief Conditional comparison instruction.
     *
     * Represents:
     * <pre>
     *   if left operation right goto jump
     * </pre>
     */
    struct CompareCodeIR final {
        std::string left;
        std::string operation;
        std::string right;
        std::string jump;

        bool operator==(const CompareCodeIR &) const = default;
    };

    /// @brief Hash specialization for CompareCodeIR.
    template<>
    struct node_hash<CompareCodeIR> {
        std::uint64_t operator()(CompareCodeIR const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.left.data(), a.left.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.operation.data(), a.operation.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.right.data(), a.right.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.jump.data(), a.jump.size()));

            return h;
        }
    };

    /**
     * @brief Print instruction.
     *
     * The type field distinguishes between integer and string output.
     */
    struct PrintCodeIR final {
        std::string type; // "string" or "int"
        std::string value;

        bool operator==(const PrintCodeIR &) const = default;
    };

    /// @brief Hash specialization for PrintCodeIR.
    template<>
    struct node_hash<PrintCodeIR> {
        std::uint64_t operator()(PrintCodeIR const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.type.data(), a.type.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.value.data(), a.value.size()));

            return h;
        }
    };

    /// @brief Compile-time tag for IR variant discrimination.
    template<typename T>
    struct ir_tag;

    template<>
    struct ir_tag<AssignmentCode> : std::integral_constant<std::uint64_t, 1> {
    };
    template<>
    struct ir_tag<JumpCode> : std::integral_constant<std::uint64_t, 2> {
    };
    template<>
    struct ir_tag<LabelCode> : std::integral_constant<std::uint64_t, 3> {
    };
    template<>
    struct ir_tag<CompareCodeIR> : std::integral_constant<std::uint64_t, 4> {
    };
    template<>
    struct ir_tag<PrintCodeIR> : std::integral_constant<std::uint64_t, 5> {
    };

    /**
     * @brief Hash functor for IRInstr.
     *
     * This combines the instruction tag with the value hash of
     * the active alternative, enabling stable deduplication.
     */
    struct IRInstrHash {
        std::uint64_t operator()(IRInstr const &ir) const noexcept {
            return std::visit(
                    [](auto const &v) -> std::uint64_t {
                        using T = std::decay_t<decltype(v)>;

                        std::uint64_t h = 14695981039346656037ull;
                        h = hash_mix(h, ir_tag<T>::value);
                        h = hash_mix(h, node_hash<T>{}(v));
                        return h;
                    },
                    ir
            );
        }
    };

    /**
     * @brief Flat pool type used for IR storage.
     *
     * This pool provides arena allocation, deduplication,
     * and pointer stability for IR instructions.
     */
    using ir_pool_t = jh::conc::flat_pool<
            IRInstr,
            jh::typed::monostate,
            IRInstrHash
    >;

    /// @brief Linear sequence of IR instructions.
    struct InterCodeArray final {
        std::vector<ir_pool_t::ptr> code;

        void append(const ir_pool_t::ptr &n) { code.push_back(n); }
    };

    /// @brief Complete IR output of a compilation unit.
    struct GeneratedIR final {
        InterCodeArray code;
        std::unordered_map<std::string, std::string> identifiers;
        std::unordered_map<std::string, std::string> constants;
    };

    /**
     * @brief IR generator from AST.
     *
     * Converts AST nodes into a flat, deduplicated IR stream
     * using static visitation-based dispatch.
     */
    class IntermediateCodeGen final {
    public:
        explicit IntermediateCodeGen(const std::shared_ptr<ast::ASTNode> &root);

        GeneratedIR get();

    private:
        std::string exec_expr(const std::shared_ptr<ast::ASTNode> &n);

        void exec_assignment(const ast::Assignment *a);

        void exec_if(const ast::IfStatement *i);

        void exec_while(const ast::WhileStatement *w);

        std::string exec_condition(const ast::Condition *c);

        void exec_print(const ast::PrintStatement *p);

        void exec_declaration(const ast::Declaration *d);

        void exec_statement(const std::shared_ptr<ast::ASTNode> &n);

        std::string nextTemp();

        std::string nextLabel();

        [[maybe_unused]] [[nodiscard]] std::string currentLabel() const;

        std::string nextStringSym();

        template<typename T>
        std::string exec_expr_node(const T &);

        [[maybe_unused]] std::string exec_expr_node(const ast::IdentifierNode &id);

        [[maybe_unused]] std::string exec_expr_node(const ast::NumberNode &num);

        [[maybe_unused]] std::string exec_expr_node(const ast::StringLiteralNode &str);

        [[maybe_unused]] std::string exec_expr_node(const ast::BinOpNode &bin);

        template<typename T>
        void exec_statement_node(const T &);

        [[maybe_unused]] void exec_statement_node(const ast::Statement &st);

        [[maybe_unused]] void exec_statement_node(const ast::IfStatement &is);

        [[maybe_unused]] void exec_statement_node(const ast::WhileStatement &wh);

        [[maybe_unused]] void exec_statement_node(const ast::PrintStatement &pr);

        [[maybe_unused]] void exec_statement_node(const ast::Declaration &de);

        [[maybe_unused]] void exec_statement_node(const ast::Assignment &asg);

    private:
        std::shared_ptr<ast::ASTNode> root;
        InterCodeArray arr;
        std::unordered_map<std::string, std::string> identifiers;
        std::unordered_map<std::string, std::string> constants;
        int tCounter{1};
        int lCounter{1};
        int sCounter{1};
    };

} // namespace pseu::ir
