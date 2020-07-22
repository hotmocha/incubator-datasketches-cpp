/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <sstream>

#include "binomial_bounds.hpp"

namespace datasketches {

template<typename A>
bool theta_sketch_experimental<A>::is_estimation_mode() const {
  return get_theta64() < theta_constants::MAX_THETA && !is_empty();
}

template<typename A>
double theta_sketch_experimental<A>::get_theta() const {
  return static_cast<double>(get_theta64()) / theta_constants::MAX_THETA;
}

template<typename A>
double theta_sketch_experimental<A>::get_estimate() const {
  return get_num_retained() / get_theta();
}

template<typename A>
double theta_sketch_experimental<A>::get_lower_bound(uint8_t num_std_devs) const {
  if (!is_estimation_mode()) return get_num_retained();
  return binomial_bounds::get_lower_bound(get_num_retained(), get_theta(), num_std_devs);
}

template<typename A>
double theta_sketch_experimental<A>::get_upper_bound(uint8_t num_std_devs) const {
  if (!is_estimation_mode()) return get_num_retained();
  return binomial_bounds::get_upper_bound(get_num_retained(), get_theta(), num_std_devs);
}

template<typename A>
string<A> theta_sketch_experimental<A>::to_string(bool detail) const {
  std::basic_ostringstream<char, std::char_traits<char>, AllocChar<A>> os;
  os << "### Theta sketch summary:" << std::endl;
  os << "   num retained entries : " << get_num_retained() << std::endl;
  os << "   seed hash            : " << get_seed_hash() << std::endl;
  os << "   empty?               : " << (is_empty() ? "true" : "false") << std::endl;
  os << "   ordered?             : " << (is_ordered() ? "true" : "false") << std::endl;
  os << "   estimation mode?     : " << (is_estimation_mode() ? "true" : "false") << std::endl;
  os << "   theta (fraction)     : " << get_theta() << std::endl;
  os << "   theta (raw 64-bit)   : " << get_theta64() << std::endl;
  os << "   estimate             : " << this->get_estimate() << std::endl;
  os << "   lower bound 95% conf : " << this->get_lower_bound(2) << std::endl;
  os << "   upper bound 95% conf : " << this->get_upper_bound(2) << std::endl;
  print_specifics(os);
  os << "### End sketch summary" << std::endl;
  if (detail) {
    os << "### Retained entries" << std::endl;
    for (const auto& hash: *this) {
      os << hash << std::endl;
    }
    os << "### End retained entries" << std::endl;
  }
  return os.str();
}

// update sketch

template<typename A>
update_theta_sketch_experimental<A>::update_theta_sketch_experimental(uint8_t lg_cur_size, uint8_t lg_nom_size, resize_factor rf,
    uint64_t theta, uint64_t seed, const A& allocator):
table_(lg_cur_size, lg_nom_size, rf, theta, seed, allocator)
{}

template<typename A>
A update_theta_sketch_experimental<A>::get_allocator() const {
  return table_.allocator_;
}

template<typename A>
bool update_theta_sketch_experimental<A>::is_empty() const {
  return table_.is_empty_;
}

template<typename A>
bool update_theta_sketch_experimental<A>::is_ordered() const {
  return false;
}

template<typename A>
uint64_t update_theta_sketch_experimental<A>::get_theta64() const {
  return table_.theta_;
}

template<typename A>
uint32_t update_theta_sketch_experimental<A>::get_num_retained() const {
  return table_.num_entries_;
}

template<typename A>
uint16_t update_theta_sketch_experimental<A>::get_seed_hash() const {
  return compute_seed_hash(table_.seed_);
}

template<typename A>
uint8_t update_theta_sketch_experimental<A>::get_lg_k() const {
  return table_.lg_nom_size_;
}

template<typename A>
auto update_theta_sketch_experimental<A>::get_rf() const -> resize_factor {
  return table_.rf_;
}

template<typename A>
void update_theta_sketch_experimental<A>::update(uint64_t value) {
  update(&value, sizeof(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(int64_t value) {
  update(&value, sizeof(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(uint32_t value) {
  update(static_cast<int32_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(int32_t value) {
  update(static_cast<int64_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(uint16_t value) {
  update(static_cast<int16_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(int16_t value) {
  update(static_cast<int64_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(uint8_t value) {
  update(static_cast<int8_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(int8_t value) {
  update(static_cast<int64_t>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(double value) {
  update(canonical_double(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(float value) {
  update(static_cast<double>(value));
}

template<typename A>
void update_theta_sketch_experimental<A>::update(const std::string& value) {
  if (value.empty()) return;
  update(value.c_str(), value.length());
}

template<typename A>
void update_theta_sketch_experimental<A>::update(const void* data, size_t length) {
  const uint64_t hash = table_.hash_and_screen(data, length);
  if (hash == 0) return;
  auto result = table_.find(hash);
  if (!result.second) {
    table_.insert(result.first, hash);
  }
}

template<typename A>
void update_theta_sketch_experimental<A>::trim() {
  table_.trim();
}

template<typename A>
auto update_theta_sketch_experimental<A>::begin() -> iterator {
  return iterator(table_.entries_, 1 << table_.lg_cur_size_, 0);
}

template<typename A>
auto update_theta_sketch_experimental<A>::end() -> iterator {
  return iterator(nullptr, 0, 1 << table_.lg_cur_size_);
}

template<typename A>
auto update_theta_sketch_experimental<A>::begin() const -> const_iterator {
  return const_iterator(table_.entries_, 1 << table_.lg_cur_size_, 0);
}

template<typename A>
auto update_theta_sketch_experimental<A>::end() const -> const_iterator {
  return const_iterator(nullptr, 0, 1 << table_.lg_cur_size_);
}
template<typename A>
compact_theta_sketch_experimental<A> update_theta_sketch_experimental<A>::compact(bool ordered) const {
  return compact_theta_sketch_experimental<A>(*this, ordered);
}

template<typename A>
void update_theta_sketch_experimental<A>::print_specifics(std::ostringstream& os) const {
  os << "   lg nominal size      : " << static_cast<int>(table_.lg_nom_size_) << std::endl;
  os << "   lg current size      : " << static_cast<int>(table_.lg_cur_size_) << std::endl;
  os << "   resize factor        : " << (1 << table_.rf_) << std::endl;
}

// builder

template<typename A>
update_theta_sketch_experimental<A>::builder::builder(const A& allocator): allocator_(allocator) {}

template<typename A>
update_theta_sketch_experimental<A> update_theta_sketch_experimental<A>::builder::build() const {
  return update_theta_sketch_experimental(this->starting_lg_size(), this->lg_k_, this->rf_, this->starting_theta(), this->seed_, allocator_);
}

// experimental compact theta sketch

template<typename A>
compact_theta_sketch_experimental<A>::compact_theta_sketch_experimental(const Base& other, bool ordered):
is_empty_(other.is_empty()),
is_ordered_(other.is_ordered()),
seed_hash_(other.get_seed_hash()),
theta_(other.get_theta64()),
entries_(other.get_allocator())
{
  entries_.reserve(other.get_num_retained());
  std::copy(other.begin(), other.end(), std::back_inserter(entries_));
  if (ordered && !other.is_ordered()) std::sort(entries_.begin(), entries_.end());
}

template<typename A>
compact_theta_sketch_experimental<A>::compact_theta_sketch_experimental(bool is_empty, bool is_ordered, uint16_t seed_hash, uint64_t theta,
    std::vector<uint64_t, A>&& entries):
is_empty_(is_empty),
is_ordered_(is_ordered),
seed_hash_(seed_hash),
theta_(theta),
entries_(std::move(entries))
{}

template<typename A>
A compact_theta_sketch_experimental<A>::get_allocator() const {
  return entries_.get_allocator();
}

template<typename A>
bool compact_theta_sketch_experimental<A>::is_empty() const {
  return is_empty_;
}

template<typename A>
bool compact_theta_sketch_experimental<A>::is_ordered() const {
  return is_ordered_;
}

template<typename A>
uint64_t compact_theta_sketch_experimental<A>::get_theta64() const {
  return theta_;
}

template<typename A>
uint32_t compact_theta_sketch_experimental<A>::get_num_retained() const {
  return entries_.size();
}

template<typename A>
uint16_t compact_theta_sketch_experimental<A>::get_seed_hash() const {
  return seed_hash_;
}

template<typename A>
auto compact_theta_sketch_experimental<A>::begin() -> iterator {
  return iterator(entries_.data(), entries_.size(), 0);
}

template<typename A>
auto compact_theta_sketch_experimental<A>::end() -> iterator {
  return iterator(nullptr, 0, entries_.size());
}

template<typename A>
auto compact_theta_sketch_experimental<A>::begin() const -> const_iterator {
  return const_iterator(entries_.data(), entries_.size(), 0);
}

template<typename A>
auto compact_theta_sketch_experimental<A>::end() const -> const_iterator {
  return const_iterator(nullptr, 0, entries_.size());
}

template<typename A>
void compact_theta_sketch_experimental<A>::print_specifics(std::ostringstream& os) const {
}

} /* namespace datasketches */
