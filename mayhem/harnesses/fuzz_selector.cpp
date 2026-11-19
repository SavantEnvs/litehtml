// CSS selector strings (element::select_one / select_all, the host's querySelector API) parsed and
// matched against a document. Input is split at the first NUL: selector first, then optional HTML
// (a fixed document is used when absent).
#include "fuzz_container.h"
#include <cstdint>
#include <cstddef>

static const char* kHtml =
    "<html><head><title>t</title></head><body class=x>"
    "<div id=a class='p q' lang=en-US data-x='1 2 3'><p>1</p><p class=q>2</p><span>s</span><p>3</p></div>"
    "<ul><li>a<li id=b>b<li>c<li>d<li>e</ul><input type=checkbox checked disabled><a href='http://x/y'>l</a>"
    "<table><tr><td>1<td>2</tr></table><section><article><h2>h</h2></article></section><div></div></body></html>";

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string in(reinterpret_cast<const char*>(data), size);
    size_t      sep = in.find('\0');
    std::string sel = in.substr(0, sep);
    fuzz_container c;
    litehtml::document::ptr doc = litehtml::document::createFromString(
        sep == std::string::npos ? std::string(kHtml) : in.substr(sep + 1), &c);
    if(!doc || !doc->root()) return 0;
    doc->render(800);
    auto all = doc->root()->select_all(sel);
    auto one = doc->root()->select_one(sel);
    for(auto& el : all) {
        if(el) el->select_all(sel);
    }
    (void)one;
    // the same selector as a stylesheet rule, so it goes through css parsing + style matching too
    doc->add_stylesheet((sel + "{color:red;display:block}").c_str(), "", nullptr);
    doc->media_changed();
    return 0;
}
