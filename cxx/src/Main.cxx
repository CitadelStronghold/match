#include <iostream>

#include "Validator.hxx"

bool validArgCount ( const int argc )
{
    if ( argc == 3 )
        return true;

    std::cout << "\033[36mUsage: \033[1mTeekValidator LOG_FILE REGEXES_FILE\033[0m"
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
        std::cerr << "\033[31mError: \033[1m" << e.what () << "\033[0m\n";
        return 2;
    }
}

int main ( const int argc, const char* const* const argv )
{
    if ( !validArgCount ( argc ) )
        return 1;

    tryValidate ( argc, argv );
}