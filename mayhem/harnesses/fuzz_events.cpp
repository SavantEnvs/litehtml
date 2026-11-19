// Dynamic document paths: mouse events (:hover/:active restyling, hit testing, anchor clicks),
// append_children_from_string DOM replacement, viewport/media changes, re-render and redraw.
#include "fuzz_container.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    fuzz_container     c;
    std::string        html = fdp.ConsumeRandomLengthString(1 << 16);
    litehtml::document::ptr doc = litehtml::document::createFromString(html, &c);
    if(!doc) return 0;
    doc->render(800);
    auto redraw = [](const litehtml::position&) {};
    litehtml::position clip(0, 0, 800, 600);
    for(int ops = 0; ops < 32 && fdp.remaining_bytes() > 0; ops++)
    {
        int x = fdp.ConsumeIntegralInRange<int>(-50, 1000);
        int y = fdp.ConsumeIntegralInRange<int>(-50, 1000);
        switch(fdp.ConsumeIntegralInRange<int>(0, 7))
        {
        case 0: doc->on_mouse_over(x, y, x, y, redraw); break;
        case 1: doc->on_lbutton_down(x, y, x, y, redraw); break;
        case 2: doc->on_lbutton_up(x, y, x, y, redraw); break;
        case 3: doc->on_mouse_leave(redraw); break;
        case 4: doc->on_button_cancel(redraw); break;
        case 5: {
            std::string frag = fdp.ConsumeRandomLengthString(4096);
            bool replace = fdp.ConsumeBool();
            if(auto root = doc->root()) {
                auto target = root;
                std::string sel = fdp.ConsumeRandomLengthString(64);
                if(auto el = root->select_one(sel)) target = el;
                doc->append_children_from_string(*target, frag.c_str(), replace);
            }
            doc->render(800);
            break;
        }
        case 6:
            c.vp_width  = x;
            c.vp_height = y;
            if(doc->media_changed()) doc->render(x);
            break;
        case 7: doc->render(x); break;
        }
        doc->draw(0, 0, 0, &clip);
    }
    return 0;
}
