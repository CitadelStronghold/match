#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <mutex>
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

    using StoredString   = std::unique_ptr< char[] >;
    using StringViewPair = std::pair< const std::string_view*, const std::string_view* >;
    using StreamPair     = std::pair< std::ifstream, std::streamoff >;

private:

    StoredString logBuffer {};
    StoredString regexesBuffer {};

    std::string_view                logString {};
    std::vector< std::string_view > logLines {};
    std::string_view                regexesString {};

    const uint64_t startTime { getCurrentNanoseconds () };
    uint64_t       readStartTime {};
    uint64_t       readEndTime {};
    uint64_t       parseStartTime {};
    uint64_t       parseEndTime {};
    uint64_t       checkStartTime {};
    uint64_t       checkEndTime {};

    std::mutex failureMutex {};

private:

    struct RegexHold
    {

        int                             result;
        std::vector< std::string_view > regexStrings;
        std::vector< std::regex >       regexes;
        std::vector< StringViewPair >   failures;

    }; // struct RegexHold

    RegexHolder< RegexHold > hold {};

private:

    struct PatternStringHolder
    {

        const std::regex*       pattern {};
        const std::string_view* patternString {};

    }; // struct PatternStringHolder

private:

    struct PatternMatchHold
    {

        size_t matches {};

        // ** Exclusion error reporting utility
        const std::string_view* firstLineOfInterest;
        // ** Utilitized inside matching loop
        const size_t startMatches {};

    }; // struct PatternMatchHold

private:

    /**
     * * splitAndParseRegexes variables
     **/

    const std::string_view* splitString;

    bool   isPastYes;
    bool   isAtEnd;
    size_t endIndex;
    size_t curStartIndex;
    // ** Reduces parameter throughput
    RegexType curType;

    std::function< void ( const char*, const size_t ) > instantiateParsedLine {};
    bool ( Validator::*matchCheckPatternFunctor ) ( const std::string_view&, const std::regex& ) const;
    void ( Validator::*matchFindingFunctor ) ( PatternMatchHold&, const PatternStringHolder& );

    void resetSplitVariables ();

public:

    [[nodiscard]] int getResult () const;

private:

    void checkArgumentsValid () const;
    void checkArgumentValid ( const uint8_t i ) const;

    [[nodiscard]] bool prepareFromArguments ();
    void               loadTargetedFiles ();

    [[nodiscard]] static StoredString  makeBufferTerminated ( const size_t bytes );
    [[nodiscard]] static std::ifstream makeStream ( const char* path );
    [[nodiscard]] static StreamPair    createStream ( const char* path );
    [[nodiscard]] static StoredString  loadText ( const char* path );

private:

    void  splitAndParse ();
    void  splitAndParse_ ();
    void  setLogInstantiator ();
    void  setRegexInstantiator ();
    void  splitLogToLines ();
    void  splitAndParseString ( const std::string_view& source );
    void  splitAndParseCharacter ( size_t& i );
    void  setSplitTarget ( const std::string_view& source );
    void  finishCurrentLine ( size_t& i );
    auto  getDistanceI ( const size_t i ) const;
    auto* getCurStartAddress () const;
    auto  getOffsetCount ( const auto distance, const auto i ) const;
    void  instantiateCurrentLine ( const size_t i );
    void  emplaceNewLog ( const char* startIt, const size_t count );
    void  emplaceNewRegex ( const char* startIt, const size_t count );
    void  startNewLine ( const size_t i );
    // ** Ignore separating characters and empty lines
    void skipPastExtraCharacters ( size_t& i );
    void skipForward ( size_t& i );

    [[nodiscard]] RegexType   getTargetType () const;
    [[nodiscard]] auto&       getTargetRegexStringsVector ();
    [[nodiscard]] auto&       getTargetRegexVector ();
    [[nodiscard]] bool        isAtEndOfParse ( const size_t i ) const;
    [[nodiscard]] bool        shouldSplitHere ( const size_t i ) const;
    [[nodiscard]] static bool isNewlineCharacter ( const char c );
    [[nodiscard]] static bool isCarriageReturnCharacter ( const char c );
    [[nodiscard]] static bool isSkippedCharacter ( const char c );
    [[nodiscard]] static bool isFilteredCharacter ( const char c );

private:

    void                 performMatches ();
    void                 performMatches_ ( const RegexType type, const int failCode, const auto memberFunctor );
    [[nodiscard]] bool   checkRegexes ( const auto memberFunctor );
    [[nodiscard]] bool   checkMatchesCountValid ( const auto& patterns, const auto matches );
    void                 prepareToMatch ( const auto memberFunctor, const auto findingFunctor );
    [[nodiscard]] size_t iteratePatternsAndLinesForMatches ( const auto& patterns, const auto& patternStrings );
    void                 performMatchesParallel ( auto& patternHolders, auto& combinedMatches );
    [[nodiscard]] auto   makeParallelMatchingFunctor ( auto& combinedMatches );
    [[nodiscard]] std::vector< PatternStringHolder > computePatternHolders (
        const auto& patterns,      //
        const auto& patternStrings //
    );
    void populatePatternHolders (
        const auto& patterns,       //
        const auto& patternStrings, //
        auto&       patternHolders  //
    );
    void               iterateLinesForPattern ( PatternMatchHold& matchHold, const PatternStringHolder& patternHolder );
    [[nodiscard]] auto getMatchFindingFunctor () const;
    [[nodiscard]] bool findMatchYes (
        PatternMatchHold&       matchHold, //
        const std::string_view& line,      //
        const std::regex&       pattern    //
    );
    [[nodiscard]] bool findMatchNo (
        PatternMatchHold&       matchHold, //
        const std::string_view& line,      //
        const std::regex&       pattern    //
    );
    void findMatches (
        PatternMatchHold&          matchHold,           //
        const PatternStringHolder& patternHolder,       //
        const auto                 matchMemberFunctor,  //
        const auto                 failureMemberFunctor //
    );
    void findMatchesYes ( PatternMatchHold& matchHold, const PatternStringHolder& patternHolder );
    void findMatchesNo ( PatternMatchHold& matchHold, const PatternStringHolder& patternHolder );
    // ? Did we fail to find a match?
    void checkYesFailure ( const PatternMatchHold& matchHold, const PatternStringHolder& patternHolder );
    // ? Did we match an exclusion?
    void checkNoFailure ( const PatternMatchHold& matchHold, const PatternStringHolder& patternHolder );
    void addFailure ( const auto* lineString, const auto* patternString );

    [[nodiscard]] bool checkYesRegex ( const std::string_view& target, const std::regex& regex ) const;
    [[nodiscard]] bool checkNoRegex ( const std::string_view& target, const std::regex& regex ) const;

private:

    [[nodiscard]] static uint64_t getCurrentNanoseconds ();
    [[nodiscard]] static double   convertToMilliseconds ( const uint64_t nanoseconds );

private:

    static inline const std::string      AnyLine = "Any Line";
    static inline const std::string_view AnyLineView { AnyLine.data (), AnyLine.size () };

private:

    void        printArguments () const;
    static void printLoaded ( const uintmax_t bytes, const char* path );
    void        printTimeTaken () const;

    void printResults () const;
    void printResult ( const char* name, const RegexType type ) const;
    void printFailures ( const RegexType type ) const;

}; // class Validator