#include "Validator.hxx"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

Validator::Validator ( const int argc, const char* const* const argv )
    : argc ( argc ),
      argv ( argv )
{
    // printArguments ();
    checkArgumentsValid ();

    loadArguments ();
}

void Validator::checkArgumentsValid () const
{
    for ( int i = 1; i < argc; ++i )
        if ( !std::filesystem::exists ( argv[i] ) )
            throw std::runtime_error ( "Argument '" + std::string { argv[i] } + "' does not exist!" );

    for ( int i = 1; i < argc; ++i )
        if ( !std::filesystem::is_regular_file ( argv[i] ) )
            throw std::runtime_error ( "Argument '" + std::string { argv[i] } + "' is not a file!" );
}

int Validator::getResult () const
{
    return result;
}

void Validator::loadArguments ()
{
    logBuffer     = loadText ( argv[1] );
    regexesBuffer = loadText ( argv[2] );

    logString     = { logBuffer.get () };
    regexesString = { regexesBuffer.get () };

    // ** They don't care what the log has in it
    if ( regexesString.empty () )
        return;

    splitAndParseRegexes ();
}

void Validator::splitAndParseRegexes ()
{
    bool   pastSplit = false;
    size_t startIndex {};

    const auto regexesSize = regexesString.size ();
    const auto endIndex    = regexesString.size () - 1;

    for ( size_t i {}; i < regexesSize; ++i )
    {
        const bool atEnd = i == endIndex;
        const bool shouldSplitHere = atEnd ||  isSplitCharacter ( regexesString[i] );

        if (shouldSplitHere)
        {
            // ** How many characters were in this regex?
            const auto distance = i - startIndex;

            if ( distance > 0 )
            {
                auto& targetVector = pastSplit ? regexes[RegexType::No] : regexes[RegexType::Yes];

                const auto* curStartIt = regexesString.data () + startIndex;

                if (!isFilteredCharacter ( *curStartIt ))
                    targetVector.emplace_back ( curStartIt, atEnd ? distance + 1 : distance );
            }

            // ** Start a new string_view
            startIndex = i + 1;

            // ** Ignore separating characters and empty lines
            size_t loops {};
            while ( startIndex <= endIndex && isSplitCharacter ( regexesString[startIndex] ) )
            {
                if ( loops > 1 || ++loops > 1 )
                    pastSplit = true;

                startIndex++;
                i++;
            }
        }
    }
}

bool Validator::isSplitCharacter ( const char c )
{
    return c == '\n' || c == '\r';
}
bool Validator::isFilteredCharacter ( const char c )
{
    return c == '#';
}

std::ifstream Validator::createStream ( const char* path )
{
    std::ifstream stream { path, std::ios::binary | std::ios::ate };

    if ( stream.fail () )
        throw std::runtime_error ( "Failed to read '" + std::string { path } + "'!" );

    return stream;
}
Validator::StoredString Validator::makeBufferTerminated ( const size_t bytes )
{
    auto buffer = std::make_unique_for_overwrite< char[] > ( bytes + 1 );

    buffer.get ()[bytes] = '\0';

    return buffer;
}
Validator::StoredString Validator::loadText ( const char* path )
{
    auto                 stream = createStream ( path );
    const auto           size   = stream.tellg ();
    const std::streamoff bytes  = size;

    auto buffer = makeBufferTerminated ( bytes );

    stream.seekg ( 0 );
    stream.read ( buffer.get (), bytes );

    printLoaded ( bytes, path );
    return buffer;
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
    std::cout << "Loaded " << bytes << " bytes from '" << path << "'\n";
}