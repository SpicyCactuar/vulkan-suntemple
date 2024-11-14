#include "zstdistream.hpp"

#include <streambuf>
#include <fstream>

#include <zstd.h>

#include "../vkutils/error.hpp"

namespace {
    class ZStdStreambuf : public std::streambuf {
    public:
        ZStdStreambuf(char const* rawPath), ~ZStdStreambuf();

    protected:
        int underflow() override;

    private:
        char* outBuffer;
        std::size_t outSize;

        char* inBuffer;
        std::size_t inSize;
        ZSTD_inBuffer inState;

        ZSTD_DCtx* zstdDContext;

        std::ifstream stream;
    };
}

ZStdIStream::ZStdIStream(const char* rawPath)
    : std::istream(nullptr),
      internal(std::make_unique<ZStdStreambuf>(rawPath)) {
    rdbuf(internal.get());
}

namespace {
    ZStdStreambuf::ZStdStreambuf(char const* rawPath)
        : stream(rawPath, std::ios::binary) {
        if (!stream.is_open()) {
            throw vkutils::Error("Unable to open '%s'", rawPath);
        }

        // Init ZStd
        outSize = ZSTD_DStreamOutSize();
        outBuffer = static_cast<char*>(operator new(outSize));

        inSize = ZSTD_DStreamInSize();
        inBuffer = static_cast<char*>(operator new(inSize));

        zstdDContext = ZSTD_createDCtx();
        if (!zstdDContext) {
            throw vkutils::Error("ZSTD_createDCtx(): returned error");
        }

        // Fill buffer once
        stream.read(inBuffer, inSize);
        if (stream.bad()) {
            throw vkutils::Error("Reading: badness happened"); // :-(
        }

        inState.src = inBuffer;
        inState.pos = 0;
        inState.size = stream.gcount();

        // Decompress once
        ZSTD_outBuffer zstdOutBuffer{outBuffer, outSize, 0};
        if (const auto ret = ZSTD_decompressStream(zstdDContext, &zstdOutBuffer, &inState);
            ZSTD_isError(ret)) {
            throw vkutils::Error("Decompression: %s", ZSTD_getErrorName(ret));
        }

        // Initialize stream buffer
        setg(outBuffer, outBuffer, outBuffer + zstdOutBuffer.pos);
    }

    ZStdStreambuf::~ZStdStreambuf() {
        operator delete(inBuffer);
        operator delete(outBuffer);

        ZSTD_freeDCtx(zstdDContext);
    }

    int ZStdStreambuf::underflow() {
        // Check decompressed buffer empty
        if (gptr() == egptr()) {
            // Check input buffer empty
            if (inState.pos == inSize) {
                stream.read(inBuffer, inSize);
                if (stream.bad()) {
                    throw vkutils::Error("Reading: badness happened");
                }

                inState.pos = 0;
                inState.size = stream.gcount();
            }

            ZSTD_outBuffer zstdOutBuffer{outBuffer, outSize, 0};
            if (const auto ret = ZSTD_decompressStream(zstdDContext, &zstdOutBuffer, &inState);
                ZSTD_isError(ret)) {
                throw vkutils::Error("Decompression: %s", ZSTD_getErrorName(ret));
            }

            setg(outBuffer, outBuffer, outBuffer + zstdOutBuffer.pos);
        }

        return gptr() == egptr()
                   ? traits_type::eof()
                   : traits_type::to_int_type(*gptr());
    }
}
