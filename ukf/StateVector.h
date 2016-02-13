/*
Copyright (C) 2016 Thiemar Pty Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef STATEVECTOR_H
#define STATEVECTOR_H

#include <limits>
#include <tuple>
#include <cstddef>
#include <utility>
#include <Eigen/Core>
#include "Config.h"

namespace UKF {

    namespace detail {

    /*
    This variable template defines the number of dimensions of a particular
    StateVector field. For a normal Eigen column vector, the default
    implementation returns the number of rows at compile time.

    Template specialisations can be used to define the size of fields which
    don't have a MaxRowsAtCompileTime member, or need a custom size for some
    reason (eg. the Eigen Quaternion classes).
    */
    template <typename T>
    constexpr std::size_t SigmaPointDimension = T::MaxRowsAtCompileTime;

    template <>
    constexpr std::size_t SigmaPointDimension<Eigen::Quaternionf> = 4;

    template <>
    constexpr std::size_t SigmaPointDimension<Eigen::Quaterniond> = 4;

    template <>
    constexpr std::size_t SigmaPointDimension<real_t> = 1;

    template <typename T>
    constexpr T Adder(T v) {
        return v;
    }

    template <typename T, typename... Args>
    constexpr T Adder(T first, Args... args) {
        return first + Adder(args...);
    }

    /*
    Get the dimension of the state vector by summing the dimension of all
    fields.
    */
    template <typename... Fields>
    constexpr std::size_t GetStateVectorDimension() {
        return Adder(SigmaPointDimension<Fields>...);
    }

    /*
    Get the offset of a particular field in a parameter pack of Field objects
    by matching the provided key parameter.
    */
    template <int Key, std::size_t Offset, typename F>
    constexpr std::size_t GetFieldOffset() {
        return Key == F::key ? Offset : std::numeric_limits<std::size_t>::max();
    }

    template <int Key, std::size_t Offset, typename F1, typename F2, typename... Fields>
    constexpr std::size_t GetFieldOffset() {
        return Key == F1::key ? Offset : GetFieldOffset<Key, Offset + SigmaPointDimension<typename F1::type>, F2, Fields...>();
    }

    /*
    Get the size of a particular field in a parameter pack of Field objects
    by matching the provided key parameter.
    */
    template <int Key, typename F>
    constexpr std::size_t GetFieldSize() {
        return Key == F::key ? SigmaPointDimension<typename F::type> : std::numeric_limits<std::size_t>::max();
    }

    template <int Key, typename F1, typename F2, typename... Fields>
    constexpr std::size_t GetFieldSize() {
        return Key == F1::key ? SigmaPointDimension<typename F1::type> : GetFieldSize<Key, F2, Fields...>();
    }

    }

template <int Key, typename T>
class Field {
public:
    using type = T;
    static const int key = Key;
};

/* Alias for the Eigen type that StateVector inherits from. */
template <typename... Fields>
using StateVectorBaseType = Eigen::Matrix<real_t, detail::GetStateVectorDimension<Fields...>(), 1>;

/*
Templated state vector class. A particular UKF implementation should
instantiate this class with a list of fields that make up the state vector.

By default, fields can be Eigen vectors (including Quaternions) or scalar
floating point types (float, double). Support for other types can be added
by specialising the SigmaPointDimension variable for the desired class.
*/
template <typename IntegratorType, typename... Fields>
class StateVector : public StateVectorBaseType<typename Fields::type...> {
private:
    using field_types = std::tuple<Fields...>;
    
    static IntegratorType integrator;

    template <int Key>
    static constexpr std::size_t offset() {
        return detail::GetFieldOffset<Key, 0, Fields...>();
    }

public:
    using Base = StateVectorBaseType<typename Fields::type...>;

    /* Inherit Eigen::Matrix constructors and assignment operators. */
    using Base::Base;
    using Base::operator=;

    template <int Key>
    auto field() {
        static_assert(offset<Key>() != std::numeric_limits<std::size_t>::max(), "Specified key not present in state vector");
        return Base::template segment<detail::GetFieldSize<Key, Fields...>()>(offset<Key>());
    }

    template <int Key>
    auto field() const {
        static_assert(offset<Key>() != std::numeric_limits<std::size_t>::max(), "Specified key not present in state vector");
        return Base::template segment<detail::GetFieldSize<Key, Fields...>()>(offset<Key>());
    }
};

}

#endif