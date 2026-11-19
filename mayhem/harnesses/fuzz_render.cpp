// HTML (+ inline/<style> CSS) -> litehtml document -> layout at two widths -> draw (with and without a
// clip) -> media change. The whole public pipeline a host runs on an untrusted page.
#include "fuzz_container.h"
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_container c;
    std::string html(reinterpret_cast<const char*>(data), size);
    litehtml::document::ptr doc = litehtml::document::createFromString(html, &c);
    if(!doc) return 0;
    doc->render(800);
    litehtml::position clip(0, 0, 800, 600);
    doc->draw(0, 0, 0, &clip);
    doc->draw(0, 0, 0, nullptr);
    doc->render(240);
    doc->draw(0, -10, -10, &clip);
    c.vp_width  = 320;
    c.vp_height = 480;
    if(doc->media_changed()) {
        doc->render(320);
        doc->draw(0, 0, 0, &clip);
    }
    return 0;
}
