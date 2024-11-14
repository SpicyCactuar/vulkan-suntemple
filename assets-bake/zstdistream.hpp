#pragma once

#include <istream>
#include <memory>

/*
 * Rapidobj fortunately allows us to feed it a custom data stream,
 * the interface for this is a std::istream. Hence, to decompress
 * stuff on the fly, we have to write a std::istream / std::streambuf adaptor.
 */
class ZStdIStream : public std::istream {
public:
    explicit ZStdIStream(const char* rawPath);

private:
    std::unique_ptr<std::streambuf> internal;
};
