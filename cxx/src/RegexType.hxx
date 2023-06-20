#pragma once

#include <cstddef>
#include <cstdint>

/**
 * * Values are used as array indices
 **/
enum RegexType : uint8_t
{

    Yes = 0,
    No  = 1,

}; // enum class RegexType
constexpr size_t TotalRegexTypes = 2;
