/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once

#include "3d/common/iStdLib/types.h"

#include "AdaptorCommon/API/igc.h"
#include "common/debug/Debug.hpp"

// Forward declarations
class AsmHash;
class NosHash;
class ShaderHash;
namespace USC
{
    struct ShaderD3D;
}


class AsmHash
{
public:
    AsmHash()
        : value(0)
    {}
    QWORD value;
};

class NosHash
{
public:
    NosHash()
        : value(0)
    {}
    QWORD value;
};

class PsoHash
{
public:
    PsoHash()
        : value(0)
    {}

    PsoHash(QWORD hash)
        : value(hash)
    {}

    QWORD value;
};

class ShaderHash
{
public:
    ShaderHash()
        : asmHash()
        , nosHash()
        , psoHash()
    {}
    QWORD getAsmHash() const { return asmHash.value; }
    QWORD getNosHash() const { return nosHash.value; }
    QWORD getPsoHash() const { return psoHash.value; }

    AsmHash asmHash;
    NosHash nosHash;
    PsoHash psoHash;
};

enum class SIMDMode : unsigned char
{
    UNKNOWN,
    SIMD1,
    SIMD2,
    SIMD4,
    SIMD8,
    SIMD16,
    SIMD32,
    END,
    BEGIN = 0
};

enum class SIMDStatus : unsigned char
{
    SIMD_BEGIN = 0,
    SIMD_PASS,
    SIMD_FUNC_FAIL,
    SIMD_PERF_FAIL,
    SIMD_END
};

inline uint16_t numLanes(SIMDMode width)
{
    switch(width)
    {
    case SIMDMode::SIMD1   : return 1;
    case SIMDMode::SIMD2   : return 2;
    case SIMDMode::SIMD4   : return 4;
    case SIMDMode::SIMD8   : return 8;
    case SIMDMode::SIMD16  : return 16;
    case SIMDMode::SIMD32  : return 32;
    case SIMDMode::UNKNOWN :
    default                : assert(0 && "unreachable"); break;
    }
    return 1;
}

inline SIMDMode lanesToSIMDMode(unsigned lanes) {
    switch (lanes) {
    case  1: return SIMDMode::SIMD1;
    case  2: return SIMDMode::SIMD2;
    case  4: return SIMDMode::SIMD4;
    case  8: return SIMDMode::SIMD8;
    case 16: return SIMDMode::SIMD16;
    case 32: return SIMDMode::SIMD32;
    default:
        break;
    }

    assert(false && "Unexpected number of lanes!");
    return SIMDMode::UNKNOWN;
}

enum class ShaderType
{
    UNKNOWN,
    VERTEX_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    GEOMETRY_SHADER,
    PIXEL_SHADER,
    COMPUTE_SHADER,
    OPENCL_SHADER,
    END,
    BEGIN = 0
};

enum class ShaderDispatchMode
{
    NOT_APPLICABLE,
    SINGLE_PATCH,
    DUAL_PATCH,
    EIGHT_PATCH,
    END,
    BEGIN = 0
};

static const char *ShaderTypeString[] = {
    "ERROR",
    "VS",
    "HS",
    "DS",
    "GS",
    "PS",
    "CS",
    "OCL",
    "ERROR"
};

static_assert(sizeof(ShaderTypeString) / sizeof(*ShaderTypeString) == static_cast<size_t>(ShaderType::END) + 1,
    "Update the array");

template <typename TIter>
class RangeWrapper
{
public:
    TIter& begin() { return m_first; }
    TIter& end() { return m_last; }

private:
    RangeWrapper( TIter first, TIter last )
        : m_first(first)
        , m_last(last)
    { }

    TIter m_first;
    TIter m_last;

    template <typename T>
    friend inline RangeWrapper<T> range( const T& first, const T& last );
};

/**
 * \brief Create a proxy to iterate over a container's nonstandard iterator pairs via c++11 range-for
 *
 * This is to be used for containers that provide multiple sets of iterators
 * such as llvm::Function, which has begin()/end() and arg_begin()/arg_end().
 * In the latter case, range-for cannot be used without such a proxy.
 *
 * \example
 *     for ( auto arg : range( pFunc->arg_begin(), pFunc->arg_end() ) )
 *     {
 *         ...
 *     }
 */
template <typename TIter>
inline RangeWrapper<TIter> range( const TIter& first, const TIter& last )
{
    return RangeWrapper<TIter>(first, last);
}

template <typename TEnum>
class EnumIter
{
public:
    EnumIter operator++(/* prefix */)
    {
        increment_me();
        return *this;
    }
    EnumIter operator++(int /* postfix */)
    {
        TEnum old(m_val);
        increment_me();
        return old;
    }
    TEnum operator*() const
    {
        return m_val;
    }
    bool operator!=(EnumIter const& other) const
    {
        return !is_equal(other);
    }
    bool operator==(EnumIter const& other) const
    {
        return is_equal(other);
    }
private:
    EnumIter(TEnum init)
        : m_val(init)
    {}
    bool is_equal(EnumIter const& other) const
    {
        return m_val == other.m_val;
    }
    void increment_me()
    {
         m_val = static_cast<TEnum>( static_cast<unsigned int>(m_val) + 1 );
    }

    template <typename T>
    friend inline RangeWrapper<EnumIter<T>> eRange(T, T);

    TEnum m_val;
};

/**
 * \brief Create a proxy to iterate over every element of an enumeration
 *
 * Assumes that TEnum follows this pattern, with no gaps between the enumerated values:
 * enum class EFoo {
 *     E1,
 *     E2,
 *     END,
 *     BEGIN = 0
 * };
 */
template <typename TEnum>
inline RangeWrapper<EnumIter<TEnum>> eRange( TEnum begin = TEnum::BEGIN, TEnum end = TEnum::END)
{
    return range( EnumIter<TEnum>(begin), EnumIter<TEnum>(end) );
}

/// Template that should be used when a static_cast of a larger integer
/// type to a smaller one is required.
/// In debug, this will check if the source argument doesn't exceed the target type range.
/// In release, it is just static_cast with no check.
///
/// Note that the cases where one needs to typecast integer types should be infrequent.
/// Could be necessary when dealing with other interfaces, otherwise think twice before
/// using it - maybe changing types or modifying the code design would be better.
template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_signed<TDst>::value && std::is_signed<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    assert(std::numeric_limits<TDst>::min() <= value &&
        value <= std::numeric_limits<TDst>::max());
    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_signed<TDst>::value && std::is_unsigned<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    assert(value <= static_cast<typename std::make_unsigned<TDst>::type>(
        std::numeric_limits<TDst>::max()));
    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_unsigned<TDst>::value && std::is_signed<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    assert(0 <= value &&
        static_cast<typename std::make_unsigned<TSrc>::type>(value) <= std::numeric_limits<TDst>::max());
    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_unsigned<TDst>::value && std::is_unsigned<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    assert(value <= std::numeric_limits<TDst>::max());
    return static_cast<TDst>(value);
}

