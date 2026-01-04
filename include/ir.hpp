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

    constexpr inline std::uint64_t hash_mix(std::uint64_t h,
                                            std::uint64_t v) noexcept {
        return (h ^ v) * 1099511628211ull;
    }

    using IRInstr = std::variant<
            AssignmentCode,
            JumpCode,
            LabelCode,
            CompareCodeIR,
            PrintCodeIR
    >;

    template<typename T>
    struct node_hash;

    struct AssignmentCode final {
        std::string var;
        std::string left;
        std::string op;    // empty if none
        std::string right; // may be empty

        bool operator==(const AssignmentCode&) const = default;
    };

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

    struct JumpCode final {
        std::string dist;

        bool operator==(const JumpCode&) const = default;
    };

    template<>
    struct node_hash<JumpCode> {
        std::uint64_t operator()(JumpCode const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.dist.data(), a.dist.size()));

            return h;
        }
    };

    struct LabelCode final {
        std::string label;

        bool operator==(const LabelCode&) const = default;
    };

    template<>
    struct node_hash<LabelCode> {
        std::uint64_t operator()(LabelCode const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.label.data(), a.label.size()));

            return h;
        }
    };

    struct CompareCodeIR final {
        std::string left;
        std::string operation;
        std::string right;
        std::string jump;

        bool operator==(const CompareCodeIR&) const = default;
    };

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

    struct PrintCodeIR final {
        std::string type; // "string" or "int"
        std::string value;

        bool operator==(const PrintCodeIR&) const = default;
    };

    template<>
    struct node_hash<PrintCodeIR> {
        std::uint64_t operator()(PrintCodeIR const &a) const noexcept {
            std::uint64_t h = 14695981039346656037ull; // FNV offset

            h = hash_mix(h, jh::meta::fnv1a64(a.type.data(), a.type.size()));
            h = hash_mix(h, jh::meta::fnv1a64(a.value.data(), a.value.size()));

            return h;
        }
    };


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

    using ir_pool_t = jh::conc::flat_pool<
            IRInstr, jh::typed::monostate, IRInstrHash
    >;

    struct InterCodeArray final {
        std::vector<ir_pool_t::ptr> code;

        void append(const ir_pool_t::ptr &n) { code.push_back(n); }
    };

    struct GeneratedIR final {
        InterCodeArray code;
        std::unordered_map<std::string, std::string> identifiers;
        std::unordered_map<std::string, std::string> constants;
    };

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
