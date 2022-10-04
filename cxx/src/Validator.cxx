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
    log_     = loadText ( argv[1] );
    regexes_ = loadText ( argv[2] );

    log     = { log_.get () };
    regexes = { regexes_.get () };
}

std::ifstream Validator::createStream ( const char* path )
{
    std::ifstream stream { path, std::ios::in | std::ios::ate };

    if ( stream.fail () )
        throw std::runtime_error ( "Failed to read '" + std::string { path } + "'!" );

    return stream;
}
Validator::StoredString Validator::makeBuffer ( const size_t bytes )
{
    auto buffer = std::make_unique_for_overwrite< char[] > ( bytes );

    buffer.get ()[bytes - 1] = '\0';

    return buffer;
}
Validator::StoredString Validator::loadText ( const char* path )
{
    auto       stream = createStream ( path );
    const auto size  = stream.tellg ();
    const std::streamoff bytes = size;

    auto buffer = makeBuffer ( bytes );

    stream.seekg ( 0 );
    stream.read ( buffer.get (), size );

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