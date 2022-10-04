#pragma once

#include <array>
#include <chrono>
#include <regex>
#include <string>
#include <vector>

#include "RegexHolder.hxx"
#include "RegexType.hxx"

class Validator final
{
public:

    Validator ( const uint8_t argc, const char* const* const argv );

public:

    const uint8_t            argc;
    const char* const* const argv;

private:

    using StoredString = std::unique_ptr< char[] >;

    StoredString logBuffer {};
    StoredString regexesBuffer {};

    std::string_view logString {};
    std::string_view regexesString {};

    RegexHolder< int >                       results {};
    RegexHolder< std::vector< std::regex > > regexes {};

    const uint64_t startTime { getCurrentNanoseconds () };
    uint64_t       readStartTime {};
    uint64_t       readEndTime {};
    uint64_t       parseStartTime {};
    uint64_t       parseEndTime {};
    uint64_t       checkStartTime {};
    uint64_t       checkEndTime {};

private:

    /**
     * * splitAndParseRegexes variables
     **/

    bool   isPastSplit {};
    bool   isAtEnd {};
    size_t endIndex {};
    size_t curStartIndex {};

public:

    [[nodiscard]] int getResult () const;

private:

    void checkArgumentsValid () const;
    void checkArgumentValid ( const uint8_t i ) const;

    [[nodiscard]] bool prepareFromArguments ();
    void               loadTargetedFiles ();

    [[nodiscard]] static StoredString                               makeBufferTerminated ( const size_t bytes );
    [[nodiscard]] static std::ifstream                              makeStream ( const char* path );
    [[nodiscard]] static std::pair< std::ifstream, std::streamoff > createStream ( const char* path );
    [[nodiscard]] static StoredString                               loadText ( const char* path );

private:

    void  splitAndParseRegexes ();
    void  splitAndParseRegexes_ ();
    void  splitAndParseRegexCharacter ( size_t& i );
    void  finishCurrentRegex ( size_t& i );
    auto  getDistanceI ( const size_t i ) const;
    auto* getCurStartAddress () const;
    auto  getOffsetEndIndex ( const auto distance ) const;
    void  instantiateCurrentRegex ( const size_t i );
    void  emplaceNewRegex ( const char* startIt, const size_t count );
    void  startNewRegex ( const size_t i );
    // ** Ignore separating characters and empty lines
    void skipPastSplitCharacters ( size_t& i );
    void skipForward ( size_t& i, size_t& loops );

    [[nodiscard]] auto&       getTargetRegexVector ();
    [[nodiscard]] bool        isAtEndOfParsedRegex ( const size_t i ) const;
    [[nodiscard]] bool        shouldSplitRegexHere ( const size_t i ) const;
    [[nodiscard]] static bool isSplitCharacter ( const char c );
    [[nodiscard]] static bool isFilteredCharacter ( const char c );

private:

    void performMatches ();
    void               performMatches_ ( const RegexType type, const int failCode, auto&& functor );
    [[nodiscard]] bool iterateRegexes ( const RegexType type, auto&& functor ) const;

    [[nodiscard]] auto makeTestFunctor ( auto&& memberFunctor ) const;

    [[nodiscard]] bool checkYesRegex ( const std::regex& regex ) const;
    [[nodiscard]] bool checkNoRegex ( const std::regex& regex ) const;

private:

    [[nodiscard]] static uint64_t getCurrentNanoseconds ();
    [[nodiscard]] static double   convertToMilliseconds ( const uint64_t nanoseconds );

private:

    void        printArguments () const;
    static void printLoaded ( const uintmax_t bytes, const char* path );
    void        printTimeTaken () const;

    void printResults () const;
    void printResult ( const char* name, const RegexType type ) const;

}; // class Validator