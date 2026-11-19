// litehtml's encoding layer (src/encodings.cpp): BOM sniffing, <meta charset> prescan, label lookup and
// every legacy decoder. First byte selects a decoder (0 = run the sniffing algorithm as createFromString does).
#include <litehtml.h>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if(size < 1) return 0;
    const int   n     = static_cast<int>(litehtml::encoding::x_user_defined) + 1;
    int         sel   = data[0] % n;
    std::string input(reinterpret_cast<const char*>(data + 1), size - 1);

    litehtml::encoding enc = static_cast<litehtml::encoding>(sel);
    if(enc == litehtml::encoding::null) {
        litehtml::estring s(input);
        litehtml::encoding_sniffing_algorithm(s);
        enc = s.encoding;
        litehtml::extract_encoding_from_meta_element(input);
        litehtml::get_encoding(input.substr(0, 64));
    }
    if(enc != litehtml::encoding::null) {
        std::string out;
        litehtml::decode(input, enc, out);
    }
    return 0;
}
