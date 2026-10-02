#include <array>
#include <iostream>
#include "arith/geometry.hpp"
using namespace mhgp10;
struct Candidate24 {arith::Wide<4> num;arith::Wide<3> den;};
struct Candidate32 {arith::Wide<5> num;arith::Wide<4> den;};
// Capacity models with the fields of generator.cpp:41-48, not product records.
template<class Level> struct RecordModel {
  std::array<u32,4> sup;u8 q,flags;u32 p,u,n_i;u64 pop_begin;u32 pop_len;Level level;
};
int main(){
 std::cout<<"{\"current_Level_bytes\":"<<sizeof(geom::Level)
 <<",\"current_record_field_model_bytes\":"<<sizeof(RecordModel<geom::Level>)
 <<",\"candidate_4_3_Level_bytes\":"<<sizeof(Candidate24)
 <<",\"candidate_5_4_Level_bytes\":"<<sizeof(Candidate32)
 <<",\"candidate_4_3_record_bytes\":"<<sizeof(RecordModel<Candidate24>)
 <<",\"candidate_5_4_record_bytes\":"<<sizeof(RecordModel<Candidate32>)
 <<",\"scope\":\"Candidate layouts only, no wide constructor or engine qualification\"}\n";
}
