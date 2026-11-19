// Known-answer layout oracle for mayhem/test.sh: renders fixed HTML documents with the deterministic
// stub container and prints element placements, selector matches and decoded text. test.sh diffs the
// output against mayhem/oracle/expected.txt. Built with the project's normal flags.
#include "fuzz_container.h"
#include <litehtml/url.h>
#include <cstdio>

static void dump(const char* name, const litehtml::document::ptr& doc, const char* sel)
{
    for(auto& el : doc->root()->select_all(sel)) {
        auto p = el->get_placement();
        std::printf("%s %s %s x=%.1f y=%.1f w=%.1f h=%.1f\n", name, sel, el->get_tagName(), (double)p.x.value(),
                    (double)p.y.value(), (double)p.width.value(), (double)p.height.value());
    }
}

int main()
{
    fuzz_container c;
    auto d1 = litehtml::document::createFromString(
        "<html><body style='margin:8px'><div id=a style='width:200px;height:50px;padding:5px'>x</div>"
        "<p id=b style='margin:10px 0'>hello world</p>"
        "<div style='display:flex'><span class=f style='width:100px'>1</span><span class=f>22</span></div>"
        "<table style='border-spacing:0'><tr><td class=c style='width:30px'>a</td><td class=c>bb</td></tr></table>"
        "<div style='display:grid;grid-template-columns:50px 1fr'><div class=g>1</div><div class=g>2</div></div>"
        "<ol><li class=l>one<li class=l>two</ol></body></html>",
        &c);
    std::printf("render=%.1f\n", (double)d1->render(800).value());
    dump("d1", d1, "#a");
    dump("d1", d1, "#b");
    dump("d1", d1, ".f");
    dump("d1", d1, ".c");
    dump("d1", d1, ".g");
    dump("d1", d1, ".l");
    std::printf("render2=%.1f\n", (double)d1->render(300).value());
    dump("d1n", d1, "#b");
    std::printf("count li:nth-child(2)=%zu\n", d1->root()->select_all("li:nth-child(2)").size());
    std::printf("count :not(span)=%zu\n", d1->root()->select_all("body :not(span)").size());

    auto d2 = litehtml::document::createFromString(
        "<style>#m{width:33px;height:10px} @media (max-width:500px){#m{width:77px}}</style><div id=m></div>", &c);
    d2->render(800);
    dump("d2", d2, "#m");
    c.vp_width = 400;
    std::printf("media_changed=%d\n", d2->media_changed() ? 1 : 0);
    d2->render(400);
    dump("d2m", d2, "#m");

    std::printf("decode=%s\n", litehtml::decode("\x82\xa0\x82\xa2", litehtml::encoding::shift_jis).c_str());
    std::printf("meta=%d\n", (int)litehtml::extract_encoding_from_meta_element("<meta charset=\"koi8-r\">"));
    std::printf("resolve=%s\n",
                litehtml::resolve(litehtml::url("http://a.com/b/c/d.html"), litehtml::url("../e/f.css?q#h")).str().c_str());
    return 0;
}
