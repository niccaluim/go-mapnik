#ifndef PNG_READER_H
#define PNG_READER_H

#include <mapnik/image_reader.hpp>

#ifdef HAVE_PNG

#include <png.h>
#include <istream>

class png_reader : public mapnik::image_reader {
public:
    png_reader(std::istream *input);
    ~png_reader();
    unsigned width() const;
    unsigned height() const;
    bool has_alpha() const;
    boost::optional<mapnik::box2d<double> > bounding_box() const;
    void read(unsigned x, unsigned y, mapnik::image_rgba8& image);
    mapnik::image_any read(unsigned x, unsigned y, unsigned width, unsigned height);

private:
    std::istream *input_;
    unsigned height_;
    unsigned width_;
    bool has_alpha_;
    int depth_;
    int color_type_;

    void start_read_(png_structpp pngpp, png_infopp infopp);
    static void user_read_fn_(png_structp pngp, png_bytep datap, png_size_t length);
};

extern "C"
{
mapnik::image_reader *png_reader_for_file(std::string const& file);
mapnik::image_reader *png_reader_for_bytes(const char* data, size_t size);
}

#endif // HAVE_PNG

#endif // PNG_READER_H
