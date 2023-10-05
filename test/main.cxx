#include <ifc/version.hxx>
#include <iostream>

int main(int, char**)
{
    std::cout << "Current format version: " << static_cast<unsigned>(ifc::CurrentFormatVersion.major) << '.'
              << static_cast<unsigned>(ifc::CurrentFormatVersion.minor) << '\n';
    return 0;
}
