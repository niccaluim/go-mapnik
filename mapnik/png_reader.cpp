#include "png_reader.h"

#ifdef HAVE_PNG

#include <fstream>
#include <sstream>

using namespace std;
using namespace mapnik;

png_reader::png_reader(istream *input) {
    input_ = input;

    png_structp pngp;
    png_infop infop;
    start_read_(&pngp, &infop);

    png_uint_32 width, height;
    png_get_IHDR(pngp, infop, &width, &height, &depth_, &color_type_, NULL, NULL, NULL);
    width_ = width;
    height_ = height;
    has_alpha_ = (color_type_ & PNG_COLOR_MASK_ALPHA) != 0;

    png_destroy_read_struct(&pngp, &infop, NULL);
}

png_reader::~png_reader() {
    delete input_;
}

unsigned png_reader::width() const {
    return width_;
}

unsigned png_reader::height() const {
    return height_;
}

bool png_reader::has_alpha() const {
    return has_alpha_;
}

boost::optional<box2d<double> > png_reader::bounding_box() const {
    return boost::optional<box2d<double> >(box2d<double>(0, 0, width_ - 1, height_ - 1));
}

void png_reader::read(unsigned x, unsigned y, image_rgba8 &image) {
    png_structp pngp;
    png_infop infop;
    start_read_(&pngp, &infop);

    // ensure we have 8-bit RGBA
    png_set_add_alpha(pngp, 0xff, PNG_FILLER_AFTER);
    png_set_expand(pngp);
    png_set_gray_to_rgb(pngp);
    png_set_strip_16(pngp);
    png_read_update_info(pngp, infop);

    unsigned h = image.height();
    unsigned w = image.width();

    if (x == y == 0 && h == height_ && w == width_) {
        png_bytep *rows = (png_bytep *) malloc(height_ * sizeof(png_bytep));
        if (!rows) {
            throw image_reader_exception("out of memory");
        }
        for (unsigned i = 0; i < h; i++) {
            rows[i] = (png_bytep) image.get_row(i);
        }
        png_read_image(pngp, rows);
        free(rows);
    } else {
        unsigned rowbytes = png_get_rowbytes(pngp, infop);
        png_bytep row = (png_bytep) malloc(width_ * rowbytes);
        if (!row) {
            throw image_reader_exception("out of memory");
        }
        for (unsigned i = 0; i < y + h; i++) {
            png_read_row(pngp, row, 0);
            if (i >= y) {
                image.set_row(i - y, (const unsigned *) (row + x * rowbytes), w);
            }
        }
        free(row);
    }

    png_destroy_read_struct(&pngp, &infop, NULL);
}

image_any png_reader::read(unsigned x, unsigned y, unsigned width, unsigned height) {
    image_rgba8 image(width, height);
    read(x, y, image);
    return image;
}

void png_reader::user_read_fn_(png_structp pngp, png_bytep datap, png_size_t length) {
    istream *in = (istream *) png_get_io_ptr(pngp);
    in->read((char *) datap, length);
}

void png_reader::start_read_(png_structpp pngpp, png_infopp infopp) {
    *pngpp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*pngpp) {
        *infopp = NULL;
        throw image_reader_exception("png_create_read_struct failed");
    }
    *infopp = png_create_info_struct(*pngpp);
    if (!*infopp) {
        png_destroy_read_struct(pngpp, NULL, NULL);
        *pngpp = NULL;
        throw image_reader_exception("png_create_info_struct failed");
    }
    input_->clear();
    input_->seekg(0);
    png_set_read_fn(*pngpp, input_, user_read_fn_);
    png_read_info(*pngpp, *infopp);
}

image_reader *png_reader_for_file(string const &file) {
    return new png_reader(new ifstream(file, ios_base::binary));
}

image_reader *png_reader_for_bytes(char const* data, size_t size) {
    string str(data, size);
    return new png_reader(new istringstream(str));
}

#endif // HAVE_PNG
