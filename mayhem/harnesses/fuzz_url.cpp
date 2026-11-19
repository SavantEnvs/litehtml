// litehtml URL parsing and resolution (src/url.cpp, src/url_path.cpp) as a container does for
// href/src/@import. Input: base URL, newline, reference URL.
#include <litehtml/url.h>
#include <litehtml/url_path.h>
#include <cstdint>
#include <cstddef>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string in(reinterpret_cast<const char*>(data), size);
    size_t      nl = in.find('\n');
    std::string a  = in.substr(0, nl);
    std::string b  = nl == std::string::npos ? std::string() : in.substr(nl + 1);
    litehtml::url base(a);
    litehtml::url ref(b);
    litehtml::url r = litehtml::resolve(base, ref);
    (void)r.str();
    litehtml::url_path_resolve(base.path(), ref.path());
    litehtml::url_path_directory_name(a);
    litehtml::url_path_base_name(a);
    litehtml::url_path_append(a, b);
    litehtml::is_url_path_absolute(b);
    return 0;
}
