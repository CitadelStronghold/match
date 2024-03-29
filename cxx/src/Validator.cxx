#include "Validator.hxx"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

Validator::Validator ( const uint8_t argc, const char* const* const argv )
    : argc ( argc ),
      argv ( argv )
{
    if ( !prepareFromArguments () )
        return;

    splitAndParse ();
    performMatches ();

    printResults ();
}

int Validator::getResult () const
{
    return hold[ RegexType::Yes ].result | hold[ RegexType::No ].result;
}

void Validator::checkArgumentsValid () const
{
    for ( uint8_t i = 1; i < argc; ++i )
        checkArgumentValid ( i );
}
void Validator::checkArgumentValid ( const uint8_t i ) const
{
    if ( !std::filesystem::exists ( argv[ i ] ) )
        throw std::runtime_error ( "Argument '" + std::string { argv[ i ] } + "' does not exist!" );

    if ( !std::filesystem::is_regular_file ( argv[ i ] ) )
        throw std::runtime_error ( "Argument '" + std::string { argv[ i ] } + "' is not a file!" );
}
bool Validator::prepareFromArguments ()
{
    readStartTime = getCurrentNanoseconds ();

    checkArgumentsValid ();
    loadTargetedFiles ();

    readEndTime = getCurrentNanoseconds ();

    // ? They don't care what the log has in it?
    return !regexesString.empty ();
}
void Validator::loadTargetedFiles ()
{
    logBuffer     = loadText ( argv[ 1 ] );
    regexesBuffer = loadText ( argv[ 2 ] );

    logString     = { logBuffer.get () };
    regexesString = { regexesBuffer.get () };
}

void Validator::splitAndParse ()
{
    parseStartTime = getCurrentNanoseconds ();

    splitAndParse_ ();

    parseEndTime = getCurrentNanoseconds ();
}
void Validator::splitAndParse_ ()
{
    setLogInstantiator ();
    splitAndParseString ( logString );

    setRegexInstantiator ();
    splitAndParseString ( regexesString );
}
void Validator::resetSplitVariables ()
{
    isPastYes     = {};
    isAtEnd       = {};
    curStartIndex = {};

    // ** The other variables are set throughout the matching process
}
void Validator::setSplitTarget ( const std::string_view& source )
{
    endIndex    = source.size () - 1;
    splitString = &source;
}
void Validator::setLogInstantiator ()
{
    instantiateParsedLine = [ this ] ( const auto* startIt, const size_t count )
    {
        emplaceNewLog ( startIt, count );
    };
}
void Validator::setRegexInstantiator ()
{
    instantiateParsedLine = [ this ] ( const auto* startIt, const size_t count )
    {
        if ( !isFilteredCharacter ( *startIt ) )
            emplaceNewRegex ( startIt, count );
    };
}
void Validator::splitAndParseString ( const std::string_view& source )
{
    resetSplitVariables ();
    setSplitTarget ( source );

    const size_t count = source.size ();
    for ( size_t i {}; i < count; ++i )
        splitAndParseCharacter ( i );
}
bool Validator::isAtEndOfParse ( const size_t i ) const
{
    return i == endIndex;
}
bool Validator::shouldSplitHere ( const size_t i ) const
{
    return isAtEnd || isSkippedCharacter ( ( *splitString )[ i ] );
}
void Validator::splitAndParseCharacter ( size_t& i )
{
    isAtEnd = isAtEndOfParse ( i );
    if ( !shouldSplitHere ( i ) )
        return;

    finishCurrentLine ( i );
}
void Validator::finishCurrentLine ( size_t& i )
{
    instantiateCurrentLine ( i );
    startNewLine ( i );

    // ** If we hit \r, go past the \n if it exists
    skipPastExtraCharacters ( i );
}
RegexType Validator::getTargetType () const
{
    return !isPastYes ? RegexType::Yes : RegexType::No;
}
auto& Validator::getTargetRegexStringsVector ()
{
    return hold[ getTargetType () ].regexStrings;
}
auto& Validator::getTargetRegexVector ()
{
    return hold[ getTargetType () ].regexes;
}
auto Validator::getDistanceI ( const size_t i ) const
{
    return i - curStartIndex;
}
auto* Validator::getCurStartAddress () const
{
    return splitString->data () + curStartIndex;
}
auto Validator::getOffsetCount ( const auto distance, const auto i ) const
{
    const bool addExtraCharacter = isAtEnd && //
                                   !isSkippedCharacter ( ( *splitString )[ i ] );

    // ** Distance being the number of characters consumed
    assert ( ( getCurStartAddress () + distance ) <= ( splitString->data () + splitString->size () ) );
    assert (
        !addExtraCharacter || //
        ( getCurStartAddress () + distance + 1 ) <= ( splitString->data () + splitString->size () )
    );

    // ** The last character is not a newline, so it needs to be included
    return addExtraCharacter ? distance + 1 : distance;
}
void Validator::instantiateCurrentLine ( const size_t i )
{
    // ** How many characters were in this line?
    const auto distance    = getDistanceI ( i );
    const bool hasDistance = distance > 0;

    if ( hasDistance || !isSkippedCharacter ( ( *splitString )[ i ] ) )
        instantiateParsedLine ( getCurStartAddress (), getOffsetCount ( distance, i ) );

    if ( !hasDistance )
        isPastYes = true; // ** We are done with Yes matches, switch to No
}
void Validator::emplaceNewLog ( const char* startIt, const size_t count )
{
    logLines.emplace_back ( startIt, count );
}
void Validator::emplaceNewRegex ( const char* startIt, const size_t count )
{
    getTargetRegexStringsVector ().emplace_back ( startIt, count );

    // * std::regex::multiline not supported by MSVC
    getTargetRegexVector ().emplace_back ( startIt, count );
}
void Validator::startNewLine ( const size_t i )
{
    curStartIndex = i + 1;
}
void Validator::skipPastExtraCharacters ( size_t& i )
{
    if ( curStartIndex <= endIndex )
        skipForward ( i );
}
void Validator::skipForward ( size_t& i )
{
    if (                                                          //
        !isCarriageReturnCharacter ( ( *splitString )[ i ] ) ||   //
        !isNewlineCharacter ( ( *splitString )[ curStartIndex ] ) //
    )
        return;

    i++;
    curStartIndex++;
}

bool Validator::isNewlineCharacter ( const char c )
{
    return c == '\n';
}
bool Validator::isCarriageReturnCharacter ( const char c )
{
    return c == '\r';
}
bool Validator::isSkippedCharacter ( const char c )
{
    return isNewlineCharacter ( c ) || isCarriageReturnCharacter ( c );
}
bool Validator::isFilteredCharacter ( const char c )
{
    return c == '#';
}

std::ifstream Validator::makeStream ( const char* path )
{
    std::ifstream stream { path, std::ios::binary | std::ios::ate };

    if ( stream.fail () )
        throw std::runtime_error ( "Failed to read '" + std::string { path } + "'!" );

    return stream;
}
Validator::StreamPair Validator::createStream ( const char* path )
{
    std::ifstream stream = makeStream ( path );

    const std::streamoff bytes = stream.tellg ();
    stream.seekg ( 0 );

    return { std::move ( stream ), bytes };
}
Validator::StoredString Validator::makeBufferTerminated ( const size_t bytes )
{
    auto buffer = std::make_unique_for_overwrite< char[] > ( bytes + 1 );

    buffer.get ()[ bytes ] = '\0';

    return buffer;
}
Validator::StoredString Validator::loadText ( const char* path )
{
    auto&& [ stream, bytes ] = createStream ( path );
    auto buffer              = makeBufferTerminated ( bytes );

    stream.read ( buffer.get (), bytes );

    printLoaded ( bytes, path );
    return buffer;
}

void Validator::addFailure ( const auto* lineString, const auto* patternString )
{
    assert ( !!lineString );
    assert ( !!patternString );

    std::scoped_lock lock { failureMutex };

    hold[ curType ].failures.emplace_back ( lineString, patternString );
}
void Validator::checkYesFailure ( const PatternMatchHold& matchHold, const PatternStringHolder& patternHolder )
{
    // ? Did we find a match this loop?
    if ( matchHold.matches != matchHold.startMatches )
        return;

    addFailure ( &AnyLineView, patternHolder.patternString );
}
void Validator::checkNoFailure ( const PatternMatchHold& matchHold, const PatternStringHolder& patternHolder )
{
    // ? Did this loop expand the number of matches by the expected amount?
    if ( matchHold.matches == ( matchHold.startMatches + logLines.size () ) )
        return;

    addFailure ( matchHold.firstLineOfInterest, patternHolder.patternString );
}
bool Validator::findMatchYes (
    PatternMatchHold&       matchHold, //
    const std::string_view& line,      //
    const std::regex&       pattern    //
)
{
    if ( !( this->*matchCheckPatternFunctor ) ( line, pattern ) )
        return true;

    matchHold.matches++;
    return false;
}
bool Validator::findMatchNo (
    PatternMatchHold&       matchHold, //
    const std::string_view& line,      //
    const std::regex&       pattern    //
)
{
    if ( ( this->*matchCheckPatternFunctor ) ( line, pattern ) )
    {
        matchHold.matches++;
        return true;
    }
    else if ( !matchHold.firstLineOfInterest )
    {
        matchHold.firstLineOfInterest = &line;
        return false;
    }

    return true;
}
void Validator::findMatches (
    PatternMatchHold&          matchHold,           //
    const PatternStringHolder& patternHolder,       //
    const auto                 matchMemberFunctor,  //
    const auto                 failureMemberFunctor //
)
{
    for ( const auto& line : logLines )
        if ( !( this->*matchMemberFunctor ) ( matchHold, line, *patternHolder.pattern ) )
            break;

    // ? Did we identify a point of failure here?
    ( this->*failureMemberFunctor ) ( matchHold, patternHolder );
}
void Validator::findMatchesYes ( PatternMatchHold& matchHold, const PatternStringHolder& patternHolder )
{
    findMatches (
        matchHold,
        patternHolder,
        &Validator::findMatchYes,
        &Validator::checkYesFailure // ? Did we fail to find a match?
    );
}
void Validator::findMatchesNo ( PatternMatchHold& matchHold, const PatternStringHolder& patternHolder )
{
    findMatches (
        matchHold,
        patternHolder,
        &Validator::findMatchNo,
        &Validator::checkNoFailure // ? Did we match an exclusion?
    );
}
auto Validator::getMatchFindingFunctor () const
{
    return curType == RegexType::Yes ? &Validator::findMatchesYes : &Validator::findMatchesNo;
}
void Validator::iterateLinesForPattern (
    PatternMatchHold&          matchHold,    //
    const PatternStringHolder& patternHolder //
)
{
    ( this->*matchFindingFunctor ) ( matchHold, patternHolder );
}
size_t Validator::iteratePatternsAndLinesForMatches ( const auto& patterns, const auto& patternStrings )
{
    std::vector< PatternStringHolder > patternHolders = computePatternHolders ( patterns, patternStrings );
    std::atomic< size_t >              combinedMatches {};

    performMatchesParallel ( patternHolders, combinedMatches );

    return combinedMatches.load ( std::memory_order_acquire );
}
auto Validator::makeParallelMatchingFunctor ( auto& combinedMatches )
{
    return [ this, &combinedMatches ] ( const auto& patternHolder )
    {
        PatternMatchHold matchHold {};

        iterateLinesForPattern ( matchHold, patternHolder );

        combinedMatches.fetch_add ( matchHold.matches, std::memory_order_acq_rel );
    };
}
void Validator::performMatchesParallel ( auto& patternHolders, auto& combinedMatches )
{
    std::for_each (
        std::execution::par,
        patternHolders.begin (),
        patternHolders.end (),
        makeParallelMatchingFunctor ( combinedMatches )
    );
}
std::vector< Validator::PatternStringHolder > Validator::computePatternHolders (
    const auto& patterns,      //
    const auto& patternStrings //
)
{
    std::vector< PatternStringHolder > patternHolders {};

    populatePatternHolders ( patterns, patternStrings, patternHolders );

    return patternHolders;
}
void Validator::populatePatternHolders ( const auto& patterns, const auto& patternStrings, auto& patternHolders )
{
    const size_t patternCount = patterns.size ();

    auto patternIt       = patterns.begin ();
    auto patternStringIt = patternStrings.begin ();

    assert ( patternCount == patternStrings.size () );

    for ( size_t i {}; i < patternCount; i++ )
        patternHolders.emplace_back ( &*( patternIt++ ), &*( patternStringIt++ ) );
}
void Validator::prepareToMatch ( const auto memberFunctor, const auto findingFunctor )
{
    matchCheckPatternFunctor = memberFunctor;
    matchFindingFunctor      = findingFunctor;
}
bool Validator::checkMatchesCountValid ( const auto& patterns, const auto matches )
{
    switch ( curType )
    {
    default:
    case RegexType::Yes:
        // ** Every inclusion was found somewhere
        return matches == patterns.size ();
    case RegexType::No:
        // ** Every exclusion did not match
        return matches == ( patterns.size () * logLines.size () );
    }
}
bool Validator::checkRegexes ( const auto memberFunctor )
{
    prepareToMatch ( memberFunctor, getMatchFindingFunctor () );

    const auto& patterns       = hold[ curType ].regexes;
    const auto& patternStrings = hold[ curType ].regexStrings;
    const auto  matches        = iteratePatternsAndLinesForMatches ( patterns, patternStrings );

    return checkMatchesCountValid ( patterns, matches );
}
void Validator::performMatches_ ( const RegexType type, const int failCode, const auto memberFunctor )
{
    curType = type;

    if ( !checkRegexes ( memberFunctor ) )
        hold[ curType ].result |= failCode;
}
void Validator::performMatches ()
{
    checkStartTime = getCurrentNanoseconds ();

    performMatches_ ( RegexType::Yes, -1, &Validator::checkYesRegex );
    performMatches_ ( RegexType::No, -2, &Validator::checkNoRegex );

    checkEndTime = getCurrentNanoseconds ();
}

bool Validator::checkYesRegex ( const std::string_view& target, const std::regex& regex ) const
{
    return !!std::regex_search ( target.begin (), target.end (), regex );
}
bool Validator::checkNoRegex ( const std::string_view& target, const std::regex& regex ) const
{
    return !std::regex_search ( target.begin (), target.end (), regex );
}

uint64_t Validator::getCurrentNanoseconds ()
{
    return std::chrono::duration< uint64_t, std::nano > (            //
               std::chrono::system_clock::now ().time_since_epoch () //
    )
        .count ();
}
double Validator::convertToMilliseconds ( const uint64_t nanoseconds )
{
    return double ( nanoseconds ) / 1'000'000.;
}

void Validator::printArguments () const
{
    std::cout << "match ";

    for ( int i = 1; i < argc; ++i )
        std::cout << argv[ i ] << ( i == argc - 1 ? "" : " " );

    std::cout << std::endl;
}
void Validator::printLoaded ( const uintmax_t bytes, const char* path )
{
    std::cout <<                                 //
        "\033[34mLoaded \033[1;34m" <<           //
        bytes <<                                 //
        " \033[0;34mcharacters from \033[1m'" << //
        path <<                                  //
        "'\033[0m" << std::endl;
}
void Validator::printTimeTaken () const
{
    std::cout << "\033[36mRead in \033[1m" << convertToMilliseconds ( readEndTime - readStartTime )
              << " \033[0;36mmilliseconds\033[0m" << std::endl;
    std::cout << "\033[36mParsed in \033[1m" << convertToMilliseconds ( parseEndTime - parseStartTime )
              << " \033[0;36mmilliseconds\033[0m" << std::endl;
    std::cout << "\033[36mChecked in \033[1m" << convertToMilliseconds ( checkEndTime - checkStartTime )
              << " \033[0;36mmilliseconds\033[0m" << std::endl;
    std::cout << "\033[36mTotal \033[1m" << convertToMilliseconds ( getCurrentNanoseconds () - startTime )
              << " \033[0;36mmilliseconds\033[0m" << std::endl;
}

void Validator::printResults () const
{
    printTimeTaken ();

    printResult ( "Matches", RegexType::Yes );
    printResult ( "Exclusions", RegexType::No );
}
void Validator::printResult ( const char* name, const RegexType type ) const
{
    const auto        passed       = hold[ type ].result == 0;
    const auto        count        = passed ? hold[ type ].regexes.size () : hold[ type ].failures.size ();
    const std::string color        = passed ? "\033[32m" : "\033[31m";
    const std::string resultString = passed ? "\033[1mPassed" : "\033[1mFailed";

    std::cout << color << count << " " << name << " " << resultString << "\033[0m" << std::endl;

    printFailures ( type );
}
void Validator::printFailures ( const RegexType type ) const
{
    for ( const auto [ line, regex ] : hold[ type ].failures )
        std::cout << "\033[1;31m" << *line << " \033[0;31m| \033[1;31m" << *regex << "\033[0m" << std::endl;
}