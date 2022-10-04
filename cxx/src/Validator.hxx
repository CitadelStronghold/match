#pragma once

#include <regex>
#include <string>
#include <vector>

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
    StoredString log_ {};
    StoredString regexes_ {};

    std::string_view log {};
    std::string_view regexes {};

public:

    [[nodiscard]] int getResult () const;

private:

    void checkArgumentsValid () const;

    void loadArguments ();

private:

    [[nodiscard]] static StoredString  makeBuffer ( const size_t bytes );
    [[nodiscard]] static std::ifstream createStream ( const char* path );
    [[nodiscard]] static StoredString  loadText ( const char* path );

private:

    void        printArguments () const;
    static void printLoaded ( const uintmax_t bytes, const char* path );

}; // class Validator