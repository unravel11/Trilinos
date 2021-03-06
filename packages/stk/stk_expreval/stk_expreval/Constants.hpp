// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of NTESS nor the names of its contributors
//       may be used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#ifndef stk_expreval_Constants_hpp
#define stk_expreval_Constants_hpp

#include <string>
#include <limits>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <cctype>

#include <stk_util/util/string_case_compare.hpp>

namespace stk {
namespace expreval {

/**
 * @brief Typedef <b>ConstantMap</b> maps a constant name to a double constant.
 * The mapping is case insensitive.
 */
using ConstantMap = std::map<std::string, double, LessCase>;

constexpr const double s_false  = 0.0;
constexpr const double s_true   = 1.0;

constexpr const double s_pi = 3.14159265358979323846;
constexpr double pi() { return s_pi; }

constexpr const double s_two_pi = 2.0 * s_pi;
constexpr double two_pi() { return s_two_pi; }

constexpr const double s_deg_to_rad = s_pi / 180.0;
constexpr const double s_rad_to_deg = 1.0 / s_deg_to_rad;
constexpr double degree_to_radian() { return s_deg_to_rad; }
constexpr double radian_to_degree() { return s_rad_to_deg; }

/**
 * @brief Member function <b>getConstantMap</b> returns a reference to the defined
 * constants.
 *
 * @return a <b>ConstantMap</b> reference to the defined constants.
 */
ConstantMap &getConstantMap();

} // namespace expreval
} // namespace stk

#endif // stk_expreval_Constants_hpp
