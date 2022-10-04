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

    splitAndParse ();
    performMatches ();

    printResults ();
}

int Validator::getResult () const
{
    return hold[RegexType::Yes].result | hold[RegexType::No].result;
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

void Validator::splitAndParse ()
{
    parseStartTime = getCurrentNanoseconds ();

    splitAndParse_ ();

    parseEndTime = getCurrentNanoseconds ();
}
void Validator::splitAndParse_ ()
{
    setLogInstantiator ();
    splitAndParse ( logString );

    setRegexInstantiator ();
    splitAndParse ( regexesString );
}
void Validator::resetSplit ()
{
    isPastSplit   = {};
    isAtEnd       = {};
    curStartIndex = {};
}
void Validator::setLogInstantiator ()
{
    instantiateParsedLine = [this] ( const auto* startIt, const size_t count )
    {
        emplaceNewLog ( startIt, count );
    };
}
void Validator::setRegexInstantiator ()
{
    instantiateParsedLine = [this] ( const auto* startIt, const size_t count )
    {
        if ( !isFilteredCharacter ( *startIt ) )
            emplaceNewRegex ( startIt, count );
    };
}
void Validator::splitAndParse ( const std::string_view& source )
{
    resetSplit ();

    const size_t count = source.size ();
    endIndex           = source.size () - 1;
    splitString        = &source;

    for ( size_t i {}; i < count; ++i )
        splitAndParseCharacter ( i );
}
bool Validator::isAtEndOfParse ( const size_t i ) const
{
    return i == endIndex;
}
bool Validator::shouldSplitHere ( const size_t i ) const
{
    return isAtEnd || isSplitCharacter ( ( *splitString )[i] );
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

    // ** Ignore separating characters and empty lines from the start of the next line
    skipPastSplitCharacters ( i );
}
RegexType Validator::getTargetType () const
{
    return isPastSplit ? RegexType::No : RegexType::Yes;
}
auto& Validator::getTargetRegexStringsVector ()
{
    return hold[getTargetType ()].regexStrings;
}
auto& Validator::getTargetRegexVector ()
{
    return hold[getTargetType ()].regexes;
}
auto Validator::getDistanceI ( const size_t i ) const
{
    return i - curStartIndex;
}
auto* Validator::getCurStartAddress () const
{
    return splitString->data () + curStartIndex;
}
auto Validator::getOffsetEndIndex ( const auto distance ) const
{
    // ** The last character is not a newline, so it needs to be included
    return isAtEnd ? distance + 1 : distance;
}
void Validator::instantiateCurrentLine ( const size_t i )
{
    // ** How many characters were in this line?
    const auto distance = getDistanceI ( i );
    if ( distance == 0 )
        return;

    instantiateParsedLine ( getCurStartAddress (), getOffsetEndIndex ( distance ) );
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
void Validator::skipPastSplitCharacters ( size_t& i )
{
    size_t loops {};

    while ( curStartIndex <= endIndex && isSplitCharacter ( ( *splitString )[curStartIndex] ) )
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

bool Validator::checkRegexes ( const RegexType type, auto&& functor )
{
    const auto&  patterns     = hold[type].regexes;
    const size_t patternCount = patterns.size ();

    size_t matches {};

    for ( size_t i {}; i < patternCount; i++ )
    {
        const auto startMatches = matches;

        // TODO ONLY ON ::No
        const std::string_view* lastLineOfInterest {};

        for ( const auto& line : logLines )
            if ( type == RegexType::Yes )
            {
                if ( functor ( line, patterns[i] ) )
                {
                    matches++;
                    break;
                }
            }
            else if ( type == RegexType::No )
            {
                if ( functor ( line, patterns[i] ) )
                {
                    matches++;
                }
                else
                {
                    lastLineOfInterest = &line;
                }
            }

        // ? Did we fail to find a match or exclusion?
        if ( type == RegexType::Yes )
        {
            if ( matches == startMatches )
                hold[type].failures.emplace_back ( &AnyLineView, &hold[type].regexStrings[i] );
        }
        else if ( type == RegexType::No )
        {
            if ( matches < ( startMatches + logLines.size () ) )
                hold[type].failures.emplace_back ( lastLineOfInterest, &hold[type].regexStrings[i] );
        }
    }

    return type == RegexType::Yes ? matches == patternCount : matches == ( patternCount * logLines.size () );
}
auto Validator::makeTestFunctor ( auto&& memberFunctor )
{
    return [this, &memberFunctor] ( auto&&... args ) -> bool
    {
        return ( this->*memberFunctor ) ( std::forward< decltype ( args ) > ( args )... );
    };
}
void Validator::performMatches_ ( const RegexType type, const int failCode, auto&& memberFunctor )
{
    if ( !checkRegexes ( type, makeTestFunctor ( memberFunctor ) ) )
        hold[type].result = failCode;

    return;
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
    return std::regex_search ( target.begin (), target.end (), regex );
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
    const auto        result       = hold[type].result;
    const std::string color        = result == 0 ? "\033[32m" : "\033[31m";
    const std::string resultString = result == 0 ? "\033[1mPassed" : "\033[1mFailed";

    std::cout << color << name << " " << resultString << "\033[0m\n";

    printFailures ( type );
}
void Validator::printFailures ( const RegexType type ) const
{
    for ( const auto [line, regex] : hold[type].failures )
        std::cout << "\033[1;31m" << *line << " \033[0;31m| \033[1;31m" << *regex << "\033[0m\n";
}