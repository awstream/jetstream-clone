#include "aggregate_avg.h"

using namespace std;
using namespace jetstream::cube;


vector<string>  MysqlAggregateAvg::get_column_types() const {
  vector<string> decl;
  decl.push_back("INT");
  decl.push_back("INT");
  return decl;
}

vector<string> MysqlAggregateAvg::get_column_names() const {
  vector<string> decl;
  decl.push_back(get_base_column_name()+"_sum");
  decl.push_back(get_base_column_name()+"_count");
  return decl;
}

string MysqlAggregateAvg::get_update_with_new_entry_sql() const {
  //VALUES() allow you to incorporate the value of the new entry as it would be if the entry was inserted as a new row;
  string sql = "`"+get_base_column_name()+"_sum` = `"+get_base_column_name()+"_sum` + VALUES(`"+get_base_column_name()+"_sum`), ";
  sql += "`"+get_base_column_name()+"_count` = `"+get_base_column_name()+"_count` + 1";
  return sql;
}

string MysqlAggregateAvg::get_update_with_partial_aggregate_sql() const {
  //VALUES() allow you to incorporate the value of the new entry as it would be if the entry was inserted as a new row;
  string sql = "`"+get_base_column_name()+"_sum` = `"+get_base_column_name()+"_sum` + VALUES(`"+get_base_column_name()+"_sum`), ";
  sql += "`"+get_base_column_name()+"_count` = `"+get_base_column_name()+"_count` +  VALUES(`"+get_base_column_name()+"_count`)";
  return sql;
}

void MysqlAggregateAvg::set_value_for_insert_entry(shared_ptr<sql::PreparedStatement> pstmt, jetstream::Tuple const &t, int &tuple_index, int &field_index) const {
  jetstream::Element * const e = const_cast<jetstream::Tuple &>(t).mutable_e(tuple_index);

  if(e->has_i_val()) {
    pstmt->setInt(field_index, e->i_val());
    pstmt->setInt(field_index+1, 1);
    tuple_index += 1;
    field_index += 2;
    return;
  }

  if(e->has_d_val()) {
    pstmt->setDouble(field_index, e->d_val());
    pstmt->setInt(field_index+1, 1);
    tuple_index += 1;
    field_index += 2;
    return;
  }

  LOG(FATAL) << "Something went wrong when processing tuple for field "<< name;
}

void MysqlAggregateAvg::set_value_for_insert_partial_aggregate(shared_ptr<sql::PreparedStatement> pstmt, jetstream::Tuple const &t, int &tuple_index, int &field_index) const {
  jetstream::Element * const e_sum = const_cast<jetstream::Tuple &>(t).mutable_e(tuple_index);
  jetstream::Element * const e_count = const_cast<jetstream::Tuple &>(t).mutable_e(tuple_index+1);

  if(!e_count->has_i_val()) {
    LOG(FATAL) << "Count not properly formatted when processing tuple for field "<< name;
    return;
  }

  if(e_sum->has_i_val()) {
    pstmt->setInt(field_index, e_sum->i_val());
    pstmt->setInt(field_index+1, e_count->i_val());
    tuple_index += 2;
    field_index += 2;
    return;
  }

  if(e_sum->has_d_val()) {
    pstmt->setDouble(field_index, e_sum->d_val());
    pstmt->setInt(field_index+1, e_count->i_val());
    tuple_index += 2;
    field_index += 2;
    return;
  }

  LOG(FATAL) << "Something went wrong when processing tuple for field "<< name;
}

void MysqlAggregateAvg::populate_tuple_final(boost::shared_ptr<jetstream::Tuple> t, boost::shared_ptr<sql::ResultSet> resultset, int &column_index) const {
  int sum = resultset->getInt(column_index);
  int count = resultset->getInt(column_index+1);
  column_index += 2;
  jetstream::Element * elem = t->add_e();
  elem->set_i_val(sum/count);
  elem->set_d_val((float)sum/(float)count);
}

void MysqlAggregateAvg::populate_tuple_partial(boost::shared_ptr<jetstream::Tuple> t, boost::shared_ptr<sql::ResultSet> resultset, int &column_index) const {
  int sum = resultset->getInt(column_index);
  int count = resultset->getInt(column_index+1);
  column_index += 2;
  jetstream::Element * elem = t->add_e();
  elem->set_i_val(sum);
  elem = t->add_e();
  elem->set_i_val(count);
}
