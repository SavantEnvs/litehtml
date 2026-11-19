// CSS stylesheet text applied to a fixed, feature-rich document. Input is split at the first NUL:
// part 1 goes through document::add_stylesheet (the host's user-stylesheet API) and also as the
// document's user_styles; part 2 is what the container returns for every @import.
#include "fuzz_container.h"
#include <cstdint>
#include <cstddef>

static const char* kHtml =
    "<!DOCTYPE html><html><head><title>t</title><style>@import url(a.css);</style></head><body>"
    "<h1 id=h class='a b'>Title</h1><p>Some <b>bold</b> <i>it</i> <a href='x'>link</a> text<br>more</p>"
    "<ul><li>one<li>two<ol start=3><li>x<li>y</ol></ul>"
    "<table border=1><caption>c</caption><tr><th colspan=2>h</th></tr><tr><td>1<td rowspan=2>2</tr><tr><td>3</tr></table>"
    "<div class=flex style='display:flex'><span>f1</span><span>f2</span><img src=i.png></div>"
    "<div class=grid style='display:grid'><div>g1</div><div>g2</div><div>g3</div></div>"
    "<div style='position:absolute;left:5px'>abs</div><pre>pre  text</pre><input type=text value=v>"
    "<span style='float:left'>fl</span><div><span>inline</span>text<div>block in inline</div></div>"
    "</body></html>";

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string in(reinterpret_cast<const char*>(data), size);
    size_t      sep = in.find('\0');
    std::string css = in.substr(0, sep);
    fuzz_container c;
    c.imported_css = sep == std::string::npos ? std::string() : in.substr(sep + 1);
    litehtml::document::ptr doc = litehtml::document::createFromString(kHtml, &c, litehtml::master_css, css);
    if(!doc) return 0;
    doc->add_stylesheet(css.c_str(), "http://example.com/", "screen");
    doc->media_changed();
    doc->render(800);
    litehtml::position clip(0, 0, 800, 600);
    doc->draw(0, 0, 0, &clip);
    doc->render(200);
    doc->draw(0, 0, 0, nullptr);
    return 0;
}
