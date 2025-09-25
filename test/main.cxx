#include <ifc/version.hxx>

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <iostream>
#endif

int main(int, char**)
{
    std::cout << "Current format version: " << static_cast<unsigned>(ifc::CurrentFormatVersion.major) << '.'
              << static_cast<unsigned>(ifc::CurrentFormatVersion.minor) << '\n';
    return 0;
}
