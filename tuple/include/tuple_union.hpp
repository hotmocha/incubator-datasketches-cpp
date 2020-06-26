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

#ifndef TUPLE_UNION_HPP_
#define TUPLE_UNION_HPP_

#include "serde.hpp"
#include "tuple_sketch.hpp"
#include "theta_union_base.hpp"

namespace datasketches {

// for types with defined + operation
template<typename Summary>
struct default_union_policy {
  void operator()(Summary& summary, const Summary& other) const {
    summary += other;
  }
};

template<
  typename Summary,
  typename Policy = default_union_policy<Summary>,
  typename SerDe = serde<Summary>,
  typename Allocator = std::allocator<Summary>
>
class tuple_union {
public:
  using Entry = std::pair<uint64_t, Summary>;
  using AllocEntry = typename std::allocator_traits<Allocator>::template rebind_alloc<Entry>;
  using ExtractKey = pair_extract_key<uint64_t, Summary>;
  using Sketch = tuple_sketch<Summary, SerDe, Allocator>;
  using CompactSketch = compact_tuple_sketch<Summary, SerDe, Allocator>;
  using resize_factor = theta_constants::resize_factor;

  // reformulate the external policy that operates on Summary
  // in terms of operations on Entry
  struct internal_policy {
    internal_policy(const Policy& policy): policy_(policy) {}
    Entry& operator()(Entry& internal_entry, const Entry& incoming_entry) const {
      policy_(internal_entry.second, incoming_entry.second);
      return internal_entry;
    }
    Policy policy_;
  };

  using State = theta_union_base<Entry, ExtractKey, internal_policy, Sketch, CompactSketch, AllocEntry>;

  // No constructor here. Use builder instead.
  class builder;

  /**
   * This method is to update the union with a given sketch
   * @param sketch to update the union with
   */
  void update(const Sketch& sketch);

  /**
   * This method produces a copy of the current state of the union as a compact sketch.
   * @param ordered optional flag to specify if ordered sketch should be produced
   * @return the result of the union
   */
  CompactSketch get_result(bool ordered = true) const;

private:
  State state_;

  // for builder
  tuple_union(uint8_t lg_cur_size, uint8_t lg_nom_size, resize_factor rf, float p, uint64_t seed, const Policy& policy);
};

template<typename S, typename P, typename SD, typename A>
class tuple_union<S, P, SD, A>::builder: public theta_base_builder<tuple_union<S, P, SD, A>::builder> {
public:
  /**
   * Creates and instance of the builder with default parameters.
   */
  builder(const P& policy = P());

  /**
   * This is to create an instance of the union with predefined parameters.
   * @return an instance of the union
   */
  tuple_union<S, P, SD, A> build() const;

private:
  P policy_;
};

} /* namespace datasketches */

#include "tuple_union_impl.hpp"

#endif
