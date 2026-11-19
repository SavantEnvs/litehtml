// HTML -> layout -> drawing through upstream's cairo container (containers/cairo: container_cairo,
// cairo_borders dashed/dotted/double/rounded borders, gradients incl. conic, clips, list markers) onto a
// cairo image surface. Fonts/text use fixed metrics (no pango); images are never loaded (get_image
// returns null), so only litehtml + container drawing code is exercised.
#include <container_cairo.h>
#include <cstdint>
#include <cstddef>
#include <cstring>

class fuzz_cairo_container : public container_cairo
{
  public:
    litehtml::uint_ptr create_font(const litehtml::font_description& d, const litehtml::document*,
                                   litehtml::font_metrics* fm) override
    {
        fm->font_size = fm->height = fm->ascent = fm->x_height = fm->ch_width = d.size;
        fm->descent = 0;
        return 1;
    }
    void              delete_font(litehtml::uint_ptr) override {}
    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr) override
    {
        return static_cast<int>(std::strlen(text)) * 8;
    }
    void draw_text(litehtml::uint_ptr hdc, const char*, litehtml::uint_ptr, litehtml::web_color color,
                   const litehtml::position& pos) override
    {
        auto* cr = (cairo_t*)hdc;
        cairo_save(cr);
        cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
        cairo_rectangle(cr, (double)pos.x, (double)pos.y, (double)pos.width, (double)pos.height);
        cairo_fill(cr);
        cairo_restore(cr);
    }
    void             load_image(const char*, const char*, bool) override {}
    void             set_caption(const char*) override {}
    void             set_base_url(const char*) override {}
    void             on_anchor_click(const char*, const litehtml::element::ptr&) override {}
    void             on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) override {}
    void             set_cursor(const char*) override {}
    void             import_css(std::string&, const std::string&, std::string&) override {}
    void             get_viewport(litehtml::position& vp) const override { vp = {0, 0, 320, 240}; }
    cairo_surface_t* get_image(const std::string&) override { return nullptr; }
    double           get_screen_dpi() const override { return 96; }
    int              get_screen_width() const override { return 320; }
    int              get_screen_height() const override { return 240; }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_cairo_container c;
    std::string html(reinterpret_cast<const char*>(data), size);
    litehtml::document::ptr doc = litehtml::document::createFromString(html, &c);
    if(!doc) return 0;
    doc->render(320);
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 320, 240);
    cairo_t*         cr      = cairo_create(surface);
    litehtml::position clip(0, 0, 320, 240);
    doc->draw((litehtml::uint_ptr)cr, 0, 0, &clip);
    doc->render(100);
    doc->draw((litehtml::uint_ptr)cr, -3, -3, nullptr);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return 0;
}
