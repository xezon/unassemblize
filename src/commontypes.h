/**
 * @file
 *
 * @brief Common types
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include <array>
#include <assert.h>
#include <cstdint>
#include <string>
#include <tcb/span.hpp>
#include <type_traits>
#include <unordered_map>

namespace unassemblize
{
enum class TriState : uint8_t
{
    False,
    True,
    NotApplicable,
};

template<typename T>
using span = tcb::span<T>;

using Address64T = uint64_t;
using Address32T = uint32_t;
using IndexT = uint32_t;

using StringToIndexMapT = std::unordered_map<std::string, IndexT>;
using MultiStringToIndexMapT = std::unordered_multimap<std::string, IndexT>;
using Address64ToIndexMapT = std::unordered_map<Address64T, IndexT>;

class Executable;
class PdbReader;
class Function;

using ConstExecutablePair = std::array<const Executable *, 2>;
using ConstPdbReaderPair = std::array<const PdbReader *, 2>;
using ConstFunctionPair = std::array<const Function *, 2>;

template<typename T, typename SizeType, SizeType Size>
struct SizedArray
{
    using size_type = std::remove_const_t<SizeType>;
    static constexpr size_type MaxSize = Size;

    using underlying_array_type = std::array<T, MaxSize>;
    using reference = typename underlying_array_type::reference;
    using const_reference = typename underlying_array_type::const_reference;

    constexpr size_type max_size() { return MaxSize; }
    size_type size() const { return m_size; }
    void set_size(size_type size)
    {
        assert(size <= MaxSize);
        m_size = size;
    }
    constexpr T *data() noexcept { return m_elements.data(); }
    constexpr const T *data() const noexcept { return m_elements.data(); }
    constexpr reference operator[](SizeType pos) { return m_elements[pos]; }
    constexpr const_reference operator[](SizeType pos) const { return m_elements[pos]; }

private:
    underlying_array_type m_elements;
    size_type m_size = 0;
};

template<typename TargetType, typename SourceType>
TargetType down_cast(SourceType value)
{
    static_assert(sizeof(TargetType) < sizeof(SourceType), "Expects smaller target type");

    if constexpr (std::is_signed_v<TargetType>)
    {
        assert(std::abs(value) < (SourceType(1) << sizeof(TargetType) * 8) / 2);
    }
    else
    {
        assert(value < (SourceType(1) << sizeof(TargetType) * 8));
    }

    return static_cast<TargetType>(value);
}

} // namespace unassemblize
