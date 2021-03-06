#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "execution/sql/value.h"
#include "parser/expression/abstract_expression.h"
#include "parser/expression/aggregate_expression.h"
#include "parser/expression/column_value_expression.h"
#include "parser/expression/comparison_expression.h"
#include "parser/expression/conjunction_expression.h"
#include "parser/expression/constant_value_expression.h"
#include "parser/expression/derived_value_expression.h"
#include "parser/expression/operator_expression.h"
#include "parser/expression/parameter_value_expression.h"
#include "parser/expression/star_expression.h"

namespace terrier::execution::compiler {

/**
 * Helper class to reduce typing and increase readability when hand crafting expressions.
 */
class ExpressionMaker {
 public:
  using OwnedExpression = std::unique_ptr<parser::AbstractExpression>;
  using OwnedAggExpression = std::unique_ptr<parser::AggregateExpression>;
  using ManagedExpression = common::ManagedPointer<parser::AbstractExpression>;
  using ManagedAggExpression = common::ManagedPointer<parser::AggregateExpression>;

  ManagedExpression MakeManaged(OwnedExpression &&expr) {
    owned_exprs_.emplace_back(std::move(expr));
    return ManagedExpression(owned_exprs_.back());
  }

  ManagedAggExpression MakeAggManaged(OwnedAggExpression &&expr) {
    owned_agg_exprs_.emplace_back(std::move(expr));
    return ManagedAggExpression(owned_agg_exprs_.back());
  }

  /**
   * Create an integer constant expression
   */
  ManagedExpression Constant(int32_t val) {
    return MakeManaged(
        std::make_unique<parser::ConstantValueExpression>(type::TypeId::INTEGER, execution::sql::Integer(val)));
  }

  /**
   * Create a floating point constant expression
   */
  ManagedExpression Constant(double val) {
    return MakeManaged(
        std::make_unique<parser::ConstantValueExpression>(type::TypeId::DECIMAL, execution::sql::Real(val)));
  }

  /**
   * Create a date constant expression
   */
  ManagedExpression Constant(int32_t year, uint32_t month, uint32_t day) {
    return MakeManaged(std::make_unique<parser::ConstantValueExpression>(
        type::TypeId::DATE, sql::DateVal(sql::Date::FromYMD(year, month, day))));
  }

  /**
   * Create a date constant expression
   */
  ManagedExpression Constant(date::year_month_day ymd) {
    auto year = static_cast<int32_t>(ymd.year());
    auto month = static_cast<uint32_t>(ymd.month());
    auto day = static_cast<uint32_t>(ymd.day());
    return Constant(year, month, day);
  }

  /**
   * Create a column value expression
   */
  ManagedExpression CVE(catalog::col_oid_t column_oid, type::TypeId type) {
    return MakeManaged(std::make_unique<parser::ColumnValueExpression>(catalog::table_oid_t(0), column_oid, type));
  }

  /**
   * Create a derived value expression
   */
  ManagedExpression DVE(type::TypeId type, int tuple_idx, int value_idx) {
    return MakeManaged(std::make_unique<parser::DerivedValueExpression>(type, tuple_idx, value_idx));
  }

  /**
   * Create a parameter value expression
   */
  ManagedExpression PVE(type::TypeId type, uint32_t param_idx) {
    return MakeManaged(std::make_unique<parser::ParameterValueExpression>(param_idx, type));
  }

  ManagedExpression Star() { return MakeManaged(std::make_unique<parser::StarExpression>()); }

  /**
   * Create a Comparison expression
   */
  ManagedExpression Comparison(parser::ExpressionType comp_type, ManagedExpression child1, ManagedExpression child2) {
    std::vector<OwnedExpression> children;
    children.emplace_back(child1->Copy());
    children.emplace_back(child2->Copy());
    return MakeManaged(std::make_unique<parser::ComparisonExpression>(comp_type, std::move(children)));
  }

  /**
   *  expression for child1 == child2
   */
  ManagedExpression ComparisonEq(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_EQUAL, child1, child2);
  }

  /**
   * Create expression for child1 == child2
   */
  ManagedExpression ComparisonNeq(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_NOT_EQUAL, child1, child2);
  }

  /**
   * Create expression for child1 < child2
   */
  ManagedExpression ComparisonLt(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_LESS_THAN, child1, child2);
  }

  /**
   * Create expression for child1 <= child2
   */
  ManagedExpression ComparisonLe(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_LESS_THAN_OR_EQUAL_TO, child1, child2);
  }

  /**
   * Create expression for child1 > child2
   */
  ManagedExpression ComparisonGt(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_GREATER_THAN, child1, child2);
  }

  /**
   * Create expression for child1 >= child2
   */
  ManagedExpression ComparisonGe(ManagedExpression child1, ManagedExpression child2) {
    return Comparison(parser::ExpressionType::COMPARE_GREATER_THAN_OR_EQUAL_TO, child1, child2);
  }

  /**
   * Create a unary operation expression
   */
  ManagedExpression Operator(parser::ExpressionType op_type, type::TypeId ret_type, ManagedExpression child) {
    std::vector<OwnedExpression> children;
    children.emplace_back(child->Copy());
    return MakeManaged(std::make_unique<parser::OperatorExpression>(op_type, ret_type, std::move(children)));
  }

  /**
   * Create a binary operation expression
   */
  ManagedExpression Operator(parser::ExpressionType op_type, type::TypeId ret_type, ManagedExpression child1,
                             ManagedExpression child2) {
    std::vector<OwnedExpression> children;
    children.emplace_back(child1->Copy());
    children.emplace_back(child2->Copy());
    return MakeManaged(std::make_unique<parser::OperatorExpression>(op_type, ret_type, std::move(children)));
  }

  /**
   * create expression for child1 + child2
   */
  ManagedExpression OpSum(ManagedExpression child1, ManagedExpression child2) {
    return Operator(parser::ExpressionType::OPERATOR_PLUS, child1->GetReturnValueType(), child1, child2);
  }

  /**
   * create expression for child1 - child2
   */
  ManagedExpression OpMin(ManagedExpression child1, ManagedExpression child2) {
    return Operator(parser::ExpressionType::OPERATOR_MINUS, child1->GetReturnValueType(), child1, child2);
  }

  /**
   * create expression for child1 * child2
   */
  ManagedExpression OpMul(ManagedExpression child1, ManagedExpression child2) {
    return Operator(parser::ExpressionType::OPERATOR_MULTIPLY, child1->GetReturnValueType(), child1, child2);
  }

  /**
   * create expression for child1 / child2
   */
  ManagedExpression OpDiv(ManagedExpression child1, ManagedExpression child2) {
    return Operator(parser::ExpressionType::OPERATOR_DIVIDE, child1->GetReturnValueType(), child1, child2);
  }

  /**
   * create expression for -child
   */
  ManagedExpression OpNeg(ManagedExpression child) {
    return Operator(parser::ExpressionType::OPERATOR_UNARY_MINUS, child->GetReturnValueType(), child);
  }

  /**
   * Create expression for child1 AND/OR child2
   */
  ManagedExpression Conjunction(parser::ExpressionType op_type, ManagedExpression child1, ManagedExpression child2) {
    std::vector<OwnedExpression> children;
    children.emplace_back(child1->Copy());
    children.emplace_back(child2->Copy());
    return MakeManaged(std::make_unique<parser::ConjunctionExpression>(op_type, std::move(children)));
  }

  /**
   * Create expression for child1 AND child2
   */
  ManagedExpression ConjunctionAnd(ManagedExpression child1, ManagedExpression child2) {
    return Conjunction(parser::ExpressionType::CONJUNCTION_AND, child1, child2);
  }

  /**
   * Create expression for child1 OR child2
   */
  ManagedExpression ConjunctionOr(ManagedExpression child1, ManagedExpression child2) {
    return Conjunction(parser::ExpressionType::CONJUNCTION_OR, child1, child2);
  }

  /**
   * Create an aggregate expression
   */
  ManagedAggExpression AggregateTerm(parser::ExpressionType agg_type, ManagedExpression child, bool distinct) {
    std::vector<OwnedExpression> children;
    children.emplace_back(child->Copy());
    return MakeAggManaged(std::make_unique<parser::AggregateExpression>(agg_type, std::move(children), distinct));
  }

  /**
   * Create a sum aggregate expression
   */
  ManagedAggExpression AggSum(ManagedExpression child, bool distinct = false) {
    return AggregateTerm(parser::ExpressionType::AGGREGATE_SUM, child, distinct);
  }

  /**
   * Create a avg aggregate expression
   */
  ManagedAggExpression AggAvg(ManagedExpression child, bool distinct = false) {
    return AggregateTerm(parser::ExpressionType::AGGREGATE_AVG, child, distinct);
  }

  /**
   * Create a count aggregate expression
   */
  ManagedAggExpression AggCount(ManagedExpression child, bool distinct = false) {
    return AggregateTerm(parser::ExpressionType::AGGREGATE_COUNT, child, distinct);
  }

 private:
  // To ease memory management
  std::vector<OwnedExpression> owned_exprs_;
  std::vector<OwnedAggExpression> owned_agg_exprs_;
};
}  // namespace terrier::execution::compiler
