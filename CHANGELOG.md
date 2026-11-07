# Changelog

All notable changes to MessageSerialize are documented in this file.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions correspond to git tags.

---

## [Unreleased]

### Added

- **`std::array<T, N>`**, the allocation-free choice for message fields on no-heap targets. It's sent exactly like a `std::vector`, so one end can use `std::vector<T>` and the other `std::array<T, N>` for the same field; reading fails with `INVALID_INPUT` if the received count isn't `N`. `N` above `MSG_SERIALIZE_MAX_CONTAINER_SIZE` is a compile error.
- **Nested containers**, e.g. `std::vector<std::vector<int>>`, `std::map<std::string, std::vector<int>>`, `std::list<std::vector<std::string>>`. Each inner container is checked against the container limit on its own.
- **Configurable limits**: `MSG_SERIALIZE_MAX_STRING_SIZE` (default 256), `MSG_SERIALIZE_MAX_CONTAINER_SIZE` (default 200) and `MSG_SERIALIZE_MAX_NESTING` (default 8). Define them before including `msg_serialize.h` or on the compiler command line. The first two are checked at compile time to be at most 65535 (16-bit length fields).
- **`NESTING_TOO_DEEP`** error: `serialize::I` objects nested deeper than `MSG_SERIALIZE_MAX_NESTING` fail that read instead of growing without bound. The nesting stack is now a fixed array, so reading allocates nothing for it.
- **Custom-allocator strings**: `std::basic_string` with any allocator serializes like `std::string`, as containers with custom allocators already did.
- **Fixed-block allocator** (`allocator/`, from [DelegateMQ](https://github.com/DelegateMQ/DelegateMQ), needing nothing else from it): `xallocator`, `stl_allocator`, and `dmq::xstring`, `xlist`, `xmap`, `xset`, `xstringstream`, ... (`allocator/xtypes.h`). CMake option `MSG_SERIALIZE_ALLOCATOR=ON` puts these types in fixed-block pools; off (the default) they are the plain `std::` types on the global heap. The wire format is the same either way. `MSG_SERIALIZE_ALLOCATOR_SAFEGUARDS=ON` adds guard words and checks to every block.
- **Unit tests** moved to `SerializeTests.cpp` and much extended (limits, error paths, truncated streams, nested and const containers, arrays, allocator types). Test checks now run in every build type, not only where `assert()` is enabled.
- **CI** builds and runs each workflow twice, with the global heap and with the fixed-block allocator.

### Changed

- **Wire change: a `char*` is sent without its terminator**, the same as `std::string`, so a string reads back the same whichever way it was sent. Reading into a `char[]` adds the terminator (and still accepts one from an older sender). An older reader that reads a current `char*` into a `char[]` gets no terminator, so update both ends together. A `char*` no longer counts a terminator against the string limit.
- **Container `write()` overloads take `const` references** (with non-const forwarders), so a `const` container can be written.
- **Reading a `std::vector` reserves the received count first**, so it allocates once instead of regrowing per element.
- `msg_serialize.h` includes `<istream>`/`<ostream>` instead of `<iostream>`.

### Fixed

- **Containers of strings didn't work.** `std::vector`/`std::list`/`std::set` of `std::string` or `std::wstring`, and `std::map` with string keys or values, didn't compile on write, and on read cast the string to `serialize::I*` (undefined behaviour). Reading a class type that doesn't derive from `serialize::I` is now a compile error, as writing already was.
- **Reading into a `char[]` could leave it unterminated**: a string sent as `std::string` has no terminator on the wire, and an empty one left the buffer untouched. The read now always terminates the buffer, and fails with `STRING_TOO_LONG` if there's no room for the terminator.
- **A `char*` of 65535 characters or more was sent as an empty string**: its length was cut to 16 bits before the size check. It's now checked first, and the write fails with `STRING_TOO_LONG`.
- **A truncated stream could leave a `char[]` with uninitialized bytes**: the read now reports the stream error and leaves an empty string.
- **Nested containers didn't compile**: container elements and map values that are themselves containers now go through the container functions.

## [1.0.2] - 2026-04-14

### Changed

- **Header renamed** from `serialize.h` to `msg_serialize.h`.
- **Wire change: little-endian wire format.** Multibyte values were sent big-endian; they are now little-endian, swapped on big-endian hosts. Data written by 1.0.1 or earlier can't be read by 1.0.2.
- **Custom allocators**: the vector, list, map and set overloads take any allocator (e.g. a fixed-block `stl_allocator`), not only `std::allocator`.
- `ParsingError` has a new `NON_SEEKABLE_STREAM` value after `STREAM_ERROR`, so the numeric values of the later codes moved up by one.
- README rewritten: key features, seekable stream requirement, and a comparison with Bitsery, MessagePack and Cereal.
- `cmake_start.bat` removed; configure with `cmake -B build .`.

### Added

- **`NON_SEEKABLE_STREAM`** error and an `is_seekable` trait: writing a user-defined object needs a seekable stream to backfill its size, and a non-seekable one now fails cleanly.
- **`read(std::istream&, char* str, size_t maxLen)`**: reading into a `char[]` takes its capacity and fails with `STRING_TOO_LONG` instead of overrunning it.
- Unit tests in `main.cpp` for primitives, strings, `char[]`, user-defined objects, versioning, containers by value and by pointer, endianness, errors, multiple objects per stream and non-seekable streams.

### Removed

- `make_serialized()` / `make_unserialized()`. Call `write()`/`read()` for each value.

## [1.0.1] - 2025-04-07

### Added

- **C++17 compile-time check** that a class type written by the generic `write()` derives from `serialize::I` (`if constexpr` + `static_assert`). C++14 builds keep the runtime path.

### Changed

- `CMakeLists.txt`: one generator-neutral configure line (`cmake -B build .`); C++14 is the minimum.

## [1.0.0] - 2025-02-18

First tagged release.

### Added

- Header-only binary serializer (`serialize.h`). User types derive from `serialize::I` and implement `write()`/`read()`.
- Primitives, `std::string`, `std::wstring`, `char*`, and `std::vector` (including `std::vector<bool>`), `std::list`, `std::map` and `std::set`, by value and by pointer (pointer containers allocate on read).
- Protocol evolution: each user-defined object carries its size, so a reader skips fields it doesn't know and leaves missing ones at their defaults.
- Endianness detection and byte swapping.
- Error and parse-progress callbacks (`setErrorHandler()`, `setParseHandler()`, `getLastError()`), with string and container limits (256 characters, 200 elements).
- Compile-time rejection of unsupported containers (`std::deque`, `std::unordered_map`, ...) and of pointers to built-in or custom types.
- `make_serialized()` / `make_unserialized()` helpers to write or read several values in one call.
- GitHub workflows for Windows, Ubuntu (GCC) and Clang.

[Unreleased]: https://github.com/endurodave/MessageSerialize/compare/v1.0.2...HEAD
[1.0.2]: https://github.com/endurodave/MessageSerialize/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/endurodave/MessageSerialize/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/endurodave/MessageSerialize/releases/tag/v1.0.0
