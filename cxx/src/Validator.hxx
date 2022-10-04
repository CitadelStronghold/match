#pragma once

#include <array>
#include <regex>
#include <string>
#include <vector>

#include "RegexType.hxx"
#include "RegexHolder.hxx"

class Validator final
{
public:

    Validator ( const int argc, const char* const* const argv );

public:

    const int                argc;
    const char* const* const argv;

private:

    int result { 0 };

    using StoredString = std::unique_ptr< char[] >;

    StoredString logBuffer {};
    StoredString regexesBuffer {};

    std::string_view logString {};
    std::string_view regexesString {};

    RegexHolder<std::vector< std::string_view >> regexes{};

public:

    [[nodiscard]] int getResult () const;

private:

    void checkArgumentsValid () const;

    void        loadArguments ();
    void        splitAndParseRegexes ();

    [[nodiscard]] static bool isSplitCharacter ( const char c );
    [[nodiscard]] static bool isFilteredCharacter ( const char c );

private:

    [[nodiscard]] static StoredString  makeBufferTerminated ( const size_t bytes );
    [[nodiscard]] static std::ifstream createStream ( const char* path );
    [[nodiscard]] static StoredString  loadText ( const char* path );

private:

    void        printArguments () const;
    static void printLoaded ( const uintmax_t bytes, const char* path );

}; // class Validator