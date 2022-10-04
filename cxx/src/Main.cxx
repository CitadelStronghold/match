#include <iostream>

#include "Validator.hxx"

bool checkArgs ( const int argc )
{
    if ( argc != 3 )
    {
        std::cout << "Usage: TeekValidator log.txt regexes.txt"
                  << "\n";
        return false;
    }

    return true;
}

int main ( const int argc, const char* const* const argv )
{
    if ( !checkArgs ( argc ) )
        return -1;

    try
    {
        Validator validator { argc, argv };
        return validator.getResult ();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Error: " << e.what () << "\n";
        return -1;
    }
}