#include "planner/plannodes/create_table_plan_node.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace terrier::planner {

common::hash_t CreateTablePlanNode::Hash() const {
  common::hash_t hash = AbstractPlanNode::Hash();

  // Namespace OI
  hash = common::HashUtil::CombineHashes(hash, common::HashUtil::Hash(namespace_oid_));

  // Table Name
  hash = common::HashUtil::CombineHashes(hash, common::HashUtil::Hash(table_name_));

  // Schema
  if (table_schema_ != nullptr) {
    hash = common::HashUtil::CombineHashes(hash, table_schema_->Hash());
  }

  // Primary Key Flag
  hash = common::HashUtil::CombineHashes(hash, common::HashUtil::Hash(has_primary_key_));

  // Primary Key Info
  if (has_primary_key_) {
    hash = common::HashUtil::CombineHashes(hash, primary_key_.Hash());
  }

  // Foreign Keys
  for (const auto &foreign_key : foreign_keys_) {
    hash = common::HashUtil::CombineHashes(hash, foreign_key.Hash());
  }

  // Unique Constraints
  for (const auto &con_unique : con_uniques_) {
    hash = common::HashUtil::CombineHashes(hash, con_unique.Hash());
  }

  // Check Constraints
  for (const auto &con_check : con_checks_) {
    hash = common::HashUtil::CombineHashes(hash, con_check.Hash());
  }

  return hash;
}

bool CreateTablePlanNode::operator==(const AbstractPlanNode &rhs) const {
  if (!AbstractPlanNode::operator==(rhs)) return false;

  auto &other = dynamic_cast<const CreateTablePlanNode &>(rhs);

  // Namespace OID
  if (namespace_oid_ != other.namespace_oid_) return false;

  // Table name
  if (table_name_ != other.table_name_) return false;

  // Schema
  if (table_schema_ != nullptr) {
    if (other.table_schema_ == nullptr) return false;
    if (*table_schema_ != *other.table_schema_) return false;
  }
  if (table_schema_ == nullptr && other.table_schema_ != nullptr) return false;

  // Has primary key
  if (has_primary_key_ != other.has_primary_key_) return false;

  // Primary Key
  if (has_primary_key_ && (primary_key_ != other.primary_key_)) return false;

  // Foreign key
  if (foreign_keys_ != other.foreign_keys_) return false;

  // Unique constraints
  if (con_uniques_ != other.con_uniques_) return false;

  // Check constraints
  if (con_checks_ != other.con_checks_) return false;

  return true;
}

nlohmann::json CreateTablePlanNode::ToJson() const {
  nlohmann::json j = AbstractPlanNode::ToJson();
  j["namespace_oid"] = namespace_oid_;
  j["table_name"] = table_name_;
  j["table_schema"] = table_schema_->ToJson();

  j["has_primary_key"] = has_primary_key_;
  if (has_primary_key_) {
    j["primary_key"] = primary_key_;
  }

  j["foreign_keys"] = foreign_keys_;
  j["con_uniques"] = con_uniques_;
  j["con_checks"] = con_checks_;
  return j;
}

std::vector<std::unique_ptr<parser::AbstractExpression>> CreateTablePlanNode::FromJson(const nlohmann::json &j) {
  std::vector<std::unique_ptr<parser::AbstractExpression>> exprs;
  auto e1 = AbstractPlanNode::FromJson(j);
  exprs.insert(exprs.end(), std::make_move_iterator(e1.begin()), std::make_move_iterator(e1.end()));
  namespace_oid_ = j.at("namespace_oid").get<catalog::namespace_oid_t>();
  table_name_ = j.at("table_name").get<std::string>();

  if (!j.at("table_schema").is_null()) {
    table_schema_ = catalog::Schema::DeserializeSchema(j.at("table_schema"));
  }

  has_primary_key_ = j.at("has_primary_key").get<bool>();
  if (has_primary_key_) {
    primary_key_ = j.at("primary_key").get<PrimaryKeyInfo>();
  }

  foreign_keys_ = j.at("foreign_keys").get<std::vector<ForeignKeyInfo>>();
  con_uniques_ = j.at("con_uniques").get<std::vector<UniqueInfo>>();
  con_checks_ = j.at("con_checks").get<std::vector<CheckInfo>>();

  return exprs;
}

}  // namespace terrier::planner
