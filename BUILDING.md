# Building with CMake

This project encourages uses of up-to-date build tools and contemporary development environments.
To build from sources, you need:

- CMake 3.26 or higher
- A C++ compiler that supports C++23 (or equivalent) which is just a bug fix of C++20
- An implementation of the C++ Core Guidelines Support Library (GSL)

## Dependencies

For a list of dependencies, please refer to [vcpkg.json](vcpkg.json).

This project depends on the [Microsoft implementation of GSL](https://github.com/microsoft/GSL), which is available on all supported platforms.  We recommend that you use your favorite package manager to install it prior to building and installing the IFC SDK from source.  If you just want to build the project and try it without installing it, then there is no need to install the GSL, but in that case the build step is a bit more involved.

On Windows platforms, we recommend using [vckpg](https://vcpkg.io/en/getting-started.html) as the C++ package manager.  Issue
```sh
$ ./vckpg install --triplet x64-windows ms-gsl
```
in the VCPKG directory, to install the GSL.  The above command assumes you are building for the `x64` target.  Replace that with the appropriate triplet if you are taregting a different platform like `arm64`.

On linux platforms, there is no commonly agreed-upon name for that package.  Here are a few common name for the Microsoft implementation of the GSL on some linux distributions if you're using the system package manager:

- arch: `microsoft-gsl`
- debian: `ms-gsl`
- fedora: `guidelines-support-library`
- gentoo: `dev-cpp/ms-gsl`
- ubuntu: `ms-gsl`

Please, consult your linux distribution manual to find the appropriate package name.

## Build

Here are the steps for building in release mode with a single-configuration
generator, as with Unix Makefiles:

```sh
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

Here are the steps for building in release mode with a multi-configuration
generator, as with MSBuild projects:

```sh
cmake -B build
cmake --build build --config Release
```

If you are using vcpkg as your C++ package manager, you might need to add `-DCMAKE_TOOLCHAIN_FILE=<path-to-your-vcpkd-root>/scripts/buildsystems/vcpkg.cmake` to the CMake command in the configuration step.

### Unit Testing

For configuring with testing enabled (only MSVC is enabled for now):

```sh
cmake -B build --preset=test-msvc
cmake --build build/test
cd build
ctest -C debug test
```

### Using vcpkg (for testing)

This project depends on [`doctest`](https://github.com/doctest/doctest) for validating the SDK.  We recommend using [`vcpkg`](https://vcpkg.io) for managing this dependency.  This project does not provide a [`builtin-baseline`](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json#builtin-baseline) in the `vcpkg.json` intentionally so that system dependencies can be relied on.  If you are not using `vcpkg` in [classic mode](https://learn.microsoft.com/en-us/vcpkg/users/classic-mode) then you must introduce your own baseline (either through [`vcpkg x-update-baseline`](https://learn.microsoft.com/en-us/vcpkg/commands/update-baseline)) or add a custom [`vcpkg-configuration.json`](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-configuration-json).  Here's an example of using the `x-update-baseline` method:

```sh
vcpkg x-update-baseline --add-initial-baseline
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<path-to>/vcpkg.cmake --preset=test-msvc
```

Note: Using this method will locally modify `/vcpkg.json` so be sure to revert it or exclude it from your commit before submitting any change for PR.

On Linux platforms please consult your package manager for alternative distributions of the C++ `doctest` framework.  Some common alternatives:

- arch: `doctest`
- debian: `doctest-dev`
- fedora: `doctest`
- gentoo: `dev-cpp/doctest`
- ubuntu: `doctest-dev`

When using an alternative source for packages be sure to consult your distribution documentation on how to make that package available for CMake.

## Install

Here is the command for installing the release mode artifacts with a
single-configuration generator:

```sh
cmake --install build
```

Here is the command for installing the release mode artifacts with a
multi-configuration generator:

```sh
cmake --install build --config Release
```

### CMake package

This project exports a CMake package to be used with the [`find_package`][2]
command of CMake:

* Package name: `Microsoft.IFC`
* Target names:
  * `Microsoft.IFC::Core`
  * `Microsoft.IFC::DOM`
  * `Microsoft.IFC::SDK` - it's recommended to use this target

Example usage:

```cmake
find_package(Microsoft.IFC REQUIRED)
# Declare the imported target as a build requirement using PRIVATE, where
# project_target is a target created in the consuming project
target_link_libraries(
    project_target PRIVATE
    Microsoft.IFC::SDK
)
```

[1]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
[2]: https://cmake.org/cmake/help/latest/command/find_package.html
