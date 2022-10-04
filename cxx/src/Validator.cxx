#include "Validator.hxx"

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

    splitAndParseRegexes ();
    performMatches ();

    printResults ();
}

int Validator::getResult () const
{
    return results[RegexType::Yes] | results[RegexType::No];
}

void Validator::checkArgumentsValid () const
{
    for ( uint8_t i = 1; i < argc; ++i )
        checkArgumentValid ( i );
}
void Validator::checkArgumentValid ( const uint8_t i ) const
{
    if ( !std::filesystem::exists ( argv[i] ) )
        throw std::runtime_error ( "Argument '" + std::string { argv[i] } + "' does not exist!" );

    if ( !std::filesystem::is_regular_file ( argv[i] ) )
        throw std::runtime_error ( "Argument '" + std::string { argv[i] } + "' is not a file!" );
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
    logBuffer     = loadText ( argv[1] );
    regexesBuffer = loadText ( argv[2] );

    logString     = { logBuffer.get () };
    regexesString = { regexesBuffer.get () };
}

void Validator::splitAndParseRegexes ()
{
    parseStartTime = getCurrentNanoseconds ();

    splitAndParseRegexes_ ();

    parseEndTime = getCurrentNanoseconds ();
}
void Validator::splitAndParseRegexes_ ()
{
    const size_t regexesSize = regexesString.size ();
    endIndex                 = regexesString.size () - 1;

    for ( size_t i {}; i < regexesSize; ++i )
        splitAndParseRegexCharacter ( i );
}
bool Validator::isAtEndOfParsedRegex ( const size_t i ) const
{
    return i == endIndex;
}
bool Validator::shouldSplitRegexHere ( const size_t i ) const
{
    return isAtEnd || isSplitCharacter ( regexesString[i] );
}
void Validator::splitAndParseRegexCharacter ( size_t& i )
{
    isAtEnd = isAtEndOfParsedRegex ( i );
    if ( !shouldSplitRegexHere ( i ) )
        return;

    finishCurrentRegex ( i );
}
void Validator::finishCurrentRegex ( size_t& i )
{
    // ** How many characters were in this regex?
    instantiateCurrentRegex ( i );

    // ** Start a new regex
    startNewRegex ( i );

    // ** Ignore separating characters and empty lines
    skipPastSplitCharacters ( i );
}
auto& Validator::getTargetRegexVector ()
{
    return isPastSplit ? regexes[RegexType::No] : regexes[RegexType::Yes];
}
auto Validator::getDistanceI ( const size_t i ) const
{
    return i - curStartIndex;
}
auto* Validator::getCurStartAddress () const
{
    return regexesString.data () + curStartIndex;
}
auto Validator::getOffsetEndIndex ( const auto distance ) const
{
    // ** The last character is not a newline, so it needs to be included
    return isAtEnd ? distance + 1 : distance;
}
void Validator::instantiateCurrentRegex ( const size_t i )
{
    if ( const auto distance = getDistanceI ( i ); distance > 0 )
        if ( const auto* curStartIt = getCurStartAddress (); !isFilteredCharacter ( *curStartIt ) )
            emplaceNewRegex ( curStartIt, getOffsetEndIndex ( distance ) );
}
void Validator::emplaceNewRegex ( const char* startIt, const size_t count )
{
    getTargetRegexVector ().emplace_back ( startIt, count );
}
void Validator::startNewRegex ( const size_t i )
{
    curStartIndex = i + 1;
}
void Validator::skipPastSplitCharacters ( size_t& i )
{
    size_t loops {};

    while ( curStartIndex <= endIndex && isSplitCharacter ( regexesString[curStartIndex] ) )
        skipForward ( i, loops );
}
void Validator::skipForward ( size_t& i, size_t& loops )
{
    if ( loops > 1 || ++loops > 1 )
        isPastSplit = true;

    i++;
    curStartIndex++;
}

bool Validator::isSplitCharacter ( const char c )
{
    return c == '\n' || c == '\r';
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
std::pair< std::ifstream, std::streamoff > Validator::createStream ( const char* path )
{
    std::ifstream stream = makeStream ( path );

    const std::streamoff bytes = stream.tellg ();
    stream.seekg ( 0 );

    return { std::move ( stream ), bytes };
}
Validator::StoredString Validator::makeBufferTerminated ( const size_t bytes )
{
    auto buffer = std::make_unique_for_overwrite< char[] > ( bytes + 1 );

    buffer.get ()[bytes] = '\0';

    return buffer;
}
Validator::StoredString Validator::loadText ( const char* path )
{
    auto&& [stream, bytes] = createStream ( path );
    auto buffer            = makeBufferTerminated ( bytes );

    stream.read ( buffer.get (), bytes );

    printLoaded ( bytes, path );
    return buffer;
}

bool Validator::iterateRegexes ( const RegexType type, auto&& functor ) const
{
    const auto&  vec   = regexes[type];
    const size_t count = vec.size ();

    for ( size_t i {}; i < count; i++ )
        if ( !functor ( vec[i] ) )
            return false;

    return true;
}
auto Validator::makeTestFunctor ( auto&& memberFunctor ) const
{
    return [this, &memberFunctor] ( const std::regex& regex ) -> bool
    {
        return ( this->*memberFunctor ) ( regex );
    };
}
template< size_t FailCode >
void Validator::performMatches_ ( const RegexType type, auto&& memberFunctor )
{
    if ( !iterateRegexes ( type, makeTestFunctor ( memberFunctor ) ) )
        results[type] = FailCode;
}
void Validator::performMatches ()
{
    checkStartTime = getCurrentNanoseconds ();

    performMatches_< -1 > ( RegexType::Yes, &Validator::checkYesRegex );
    performMatches_< -2 > ( RegexType::No, &Validator::checkNoRegex );

    checkEndTime = getCurrentNanoseconds ();
}

bool Validator::checkYesRegex ( const std::regex& regex ) const
{
    // std::regex_search ( logString.begin (), logString.end (), regex );

    return true;
}
bool Validator::checkNoRegex ( const std::regex& regex ) const
{
    return true;
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
    std::cout << "TeekValidator ";

    for ( int i = 1; i < argc; ++i )
        std::cout << argv[i] << ( i == argc - 1 ? "" : " " );

    std::cout << "\n";
}
void Validator::printLoaded ( const uintmax_t bytes, const char* path )
{
    std::cout <<                                 //
        "\033[34mLoaded \033[1;34m" <<           //
        bytes <<                                 //
        " \033[0;34mcharacters from \033[1m'" << //
        path <<                                  //
        "'\033[0m\n";
}
void Validator::printTimeTaken () const
{
    std::cout << "\033[36mRead in \033[1m" << convertToMilliseconds ( readEndTime - readStartTime )
              << " \033[0;36mmilliseconds\033[0m\n";
    std::cout << "\033[36mParsed in \033[1m" << convertToMilliseconds ( parseEndTime - parseStartTime )
              << " \033[0;36mmilliseconds\033[0m\n";
    std::cout << "\033[36mChecked in \033[1m" << convertToMilliseconds ( checkEndTime - checkStartTime )
              << " \033[0;36mmilliseconds\033[0m\n";
    std::cout << "\033[36mTotal \033[1m" << convertToMilliseconds ( getCurrentNanoseconds () - startTime )
              << " \033[0;36mmilliseconds\033[0m\n";
}

void Validator::printResults () const
{
    printTimeTaken ();

    printResult ( "Matches", RegexType::Yes );
    printResult ( "Exclusions", RegexType::No );
}
void Validator::printResult ( const char* name, const RegexType type ) const
{
    const auto        result       = results[type];
    const std::string color        = result == 0 ? "\033[32m" : "\033[31m";
    const std::string resultString = result == 0 ? "\033[1mPassed" : "\033[1mFailed";

    std::cout << color << name << " " << resultString << "\033[0m\n";
}