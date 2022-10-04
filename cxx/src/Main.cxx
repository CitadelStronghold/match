#include <iostream>

#include "Validator.hxx"

bool validArgCount ( const int argc )
{
    if ( argc == 3 )
        return true;

    std::cout << "Usage: TeekValidator LOG_FILE REGEXES_FILE"
              << "\n";
    return false;
}

int tryValidate ( const int argc, const char* const* const argv )
{
    try
    {
        return Validator { uint8_t ( argc ), argv }.getResult ();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Error: " << e.what () << "\n";
        return -1;
    }
}

int main ( const int argc, const char* const* const argv )
{
    if ( !validArgCount ( argc ) )
        return -1;

    tryValidate ( argc, argv );
}