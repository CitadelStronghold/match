#pragma once

#include "RegexType.hxx"

#include <array>

template< typename T >
struct RegexHolder final
{

    RegexHolder () = default;

    RegexHolder ( const RegexHolder& )                = delete;
    RegexHolder ( RegexHolder&& ) noexcept            = delete;
    RegexHolder& operator= ( const RegexHolder& )     = delete;
    RegexHolder& operator= ( RegexHolder&& ) noexcept = delete;

private:

    std::array< T, TotalRegexTypes > values {};

public:

    [[nodiscard]] T& operator[] ( const RegexType type )
    {
        return values[type];
    }
    [[nodiscard]] const T& operator[] ( const RegexType type ) const
    {
        return values[type];
    }

}; // struct RegexHolder