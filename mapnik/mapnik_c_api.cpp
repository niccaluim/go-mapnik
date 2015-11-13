#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#include <mapnik/color.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/grid/grid.hpp>
#include <mapnik/grid/grid_renderer.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/image_reader.hpp>
#include <json/json.h>

// see https://github.com/mapnik/mapnik/issues/811
#ifdef HAVE_PNG
#include "png_reader.h"
#endif

#if MAPNIK_VERSION >= 300000
#include <mapnik/image.hpp>
#define mapnik_image_type mapnik::image_rgba8
#else
#include <mapnik/graphics.hpp>
#define mapnik_image_type mapnik::image_32
#endif


#include "mapnik_c_api.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

int mapnik_register_datasources(const char* path, char** err) {
    try {
#if MAPNIK_VERSION >= 200200
        mapnik::datasource_cache::instance().register_datasources(path);
#else
        mapnik::datasource_cache::instance()->register_datasources(path);
#endif
        return 0;
    } catch (std::exception const& ex) {
        if (err != NULL) {
            *err = strdup(ex.what());
        }
        return -1;
    }
}

int mapnik_register_fonts(const char* path, char** err) {
    try {
        mapnik::freetype_engine::register_fonts(path);
        return 0;
    } catch (std::exception const& ex) {
        if (err != NULL) {
            *err = strdup(ex.what());
        }
        return -1;
    }
}

struct _mapnik_grid_t {
    mapnik::grid * g;
};

void mapnik_grid_free(mapnik_grid_t * g) {
    if (g) {
        if (g->g) delete g->g;
        delete g;
    }
}

void utf8_append(std::string& s, unsigned cp) {
    if (cp <= 0x7f) {
        s += (char) cp;
    } else if (cp <= 0x7ff) {
        s += (char) (0xc0 | (cp >> 6));
        s += (char) (0x80 | (cp & 0x3f));
    } else if (cp <= 0xffff) {
        s += (char) (0xe0 | (cp >> 12));
        s += (char) (0x80 | ((cp >> 6) & 0x3f));
        s += (char) (0x80 | (cp & 0x3f));
    } else if (cp <= 0x1fffff) {
        s += (char) (0xf0 | (cp >> 18));
        s += (char) (0x80 | ((cp >> 12) & 0x3f));
        s += (char) (0x80 | ((cp >> 6) & 0x3f));
        s += (char) (0x80 | (cp & 0x3f));
    }
}

char * mapnik_grid_to_json(mapnik_grid_t * g, unsigned res) {
    char * json = NULL;
    if (g && g->g) {
        Json::Value root(Json::objectValue);
        root["keys"] = Json::Value(Json::arrayValue);
        root["data"] = Json::Value(Json::objectValue);
        root["grid"] = Json::Value(Json::arrayValue);

        using feature_keys_type = std::map<mapnik::value_integer, std::string>;
        feature_keys_type feature_keys = g->g->get_feature_keys();
        feature_keys_type::const_iterator feature_key_itr;

        using keys_type = std::map<mapnik::grid::lookup_type, mapnik::grid::value_type>;
        keys_type keys;
        unsigned codepoint = ' ';

        for (std::size_t y = 0; y < g->g->data().height(); y=y+res) {
            const mapnik::value_integer * row = g->g->get_row(y);
            std::string s;
            for (std::size_t x = 0; x < g->g->data().width(); x=x+res) {
                feature_key_itr = feature_keys.find(row[x]);
                if (feature_key_itr == feature_keys.end()) continue;

                mapnik::grid::lookup_type key = feature_key_itr->second;
                keys_type::iterator key_itr = keys.find(key);
                if (key_itr == keys.end()) {
                    if (row[x] == mapnik::grid::base_mask) {
                        keys[""] = codepoint;
                        root["keys"].append("");
                    } else {
                        keys[key] = codepoint;
                        root["keys"].append(key);
                    }
                    utf8_append(s, codepoint);
                    codepoint++;
                    if (codepoint == '"' || codepoint == '\\') codepoint++;
                } else {
                    utf8_append(s, key_itr->second);
                }
            }
            root["grid"].append(s);
        }

        using features_type = std::map<mapnik::grid::lookup_type, mapnik::feature_ptr>;
        features_type const& features = g->g->get_grid_features();
        for (keys_type::iterator itr = keys.begin(); itr != keys.end(); itr++) {
            features_type::const_iterator feature_itr = features.find(itr->first);
            if (feature_itr == features.end()) continue;
            mapnik::feature_ptr feature = feature_itr->second;

            Json::Value feature_json = Json::Value(Json::objectValue);
            for (std::string const& field : g->g->get_fields()) {
                if (feature->has_key(field)) {
                    mapnik::feature_impl::value_type const& v = feature->get(field);
                    switch(v.which()) { // :(
                    case 0:
                        // null value; do nothing
                        break;
                    case 1:
                        feature_json[field] = Json::Value(v.to_bool());
                        break;
                    case 2:
                        feature_json[field] = Json::Value((Json::LargestInt) v.to_int());
                        break;
                    case 3:
                        feature_json[field] = Json::Value(v.to_double());
                        break;
                    case 4:
                        feature_json[field] = Json::Value(v.to_string());
                        break;
                    }
                }
            }
            root["data"][itr->first] = feature_json;
        }

        Json::StreamWriterBuilder wbuilder;
        wbuilder["commentStyle"] = "None";
        wbuilder["indentation"] = "";
        std::string s = Json::writeString(wbuilder, root);
        json = (char *) malloc(s.size()+1);
        memcpy(json, s.c_str(), s.size()+1);
    }
    return json;
}

struct _mapnik_map_t {
    mapnik::Map * m;
    std::string * err;
};

struct _mapnik_layer_t {
    mapnik::layer *l;
};

mapnik_map_t * mapnik_map(unsigned width, unsigned height) {
    mapnik_map_t * map = new mapnik_map_t;
    map->m = new mapnik::Map(width,height);
    map->err = NULL;
    return map;
}

void mapnik_map_free(mapnik_map_t * m) {
    if (m)  {
        if (m->m) delete m->m;
        if (m->err) delete m->err;
        delete m;
    }
}

inline void mapnik_map_reset_last_error(mapnik_map_t *m) {
    if (m && m->err) { delete m->err; m->err = NULL; }
}

const char * mapnik_map_get_srs(mapnik_map_t * m) {
    if (m && m->m) return m->m->srs().c_str();
    return NULL;
}

int mapnik_map_set_srs(mapnik_map_t * m, const char* srs) {
    if (m) {
        m->m->set_srs(srs);
        return 0;
    }
    return -1;
}

int mapnik_map_load(mapnik_map_t * m, const char* stylesheet) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik::load_map(*m->m,stylesheet);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

int mapnik_map_load_string(mapnik_map_t * m, const char* stylesheet_string) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik::load_map_string(*m->m, stylesheet_string);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

int mapnik_map_zoom_all(mapnik_map_t * m) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            m->m->zoom_all();
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

void ensure_readers_registered() {
    static bool registered = false;
    if (!registered) {
        #ifdef HAVE_PNG
        factory<image_reader, string, string const&>::instance().unregister_product("png");
        factory<image_reader, string, char const*, size_t>::instance().unregister_product("png");
        register_image_reader("png", png_reader_for_file);
        register_image_reader("png", png_reader_for_bytes);
        #endif
        registered = true;
    }
}

int mapnik_map_render_to_file(mapnik_map_t * m, const char* filepath) {
    ensure_readers_registered();
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik_image_type buf(m->m->width(),m->m->height());
            mapnik::agg_renderer<mapnik_image_type> ren(*m->m,buf);
            ren.apply();
            mapnik::save_to_file(buf,filepath);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}


void mapnik_map_resize(mapnik_map_t *m, unsigned int width, unsigned int height) {
    if (m&& m->m) {
        m->m->resize(width, height);
    }
}


MAPNIKCAPICALL void mapnik_map_set_buffer_size(mapnik_map_t * m, int buffer_size) {
    m->m->set_buffer_size(buffer_size);
}

const char *mapnik_map_last_error(mapnik_map_t *m) {
    if (m && m->err) {
        return m->err->c_str();
    }
    return NULL;
}

struct _mapnik_projection_t {
    mapnik::projection * p;
};

mapnik_projection_t * mapnik_map_projection(mapnik_map_t *m) {
    mapnik_projection_t * proj = new mapnik_projection_t;
    if (m && m->m)
        proj->p = new mapnik::projection(m->m->srs());
    else
        proj->p = NULL;
    return proj;
}


void mapnik_projection_free(mapnik_projection_t *p) {
    if (p) {
        if (p->p) delete p->p;
        delete p;
    }
}


mapnik_coord_t mapnik_projection_forward(mapnik_projection_t *p, mapnik_coord_t c) {
    if (p && p->p) {
        p->p->forward(c.x, c.y);
    }
    return c;
}

struct _mapnik_bbox_t {
    mapnik::box2d<double> b;
};

mapnik_bbox_t * mapnik_bbox(double minx, double miny, double maxx, double maxy) {
    mapnik_bbox_t * b = new mapnik_bbox_t;
    b->b = mapnik::box2d<double>(minx, miny, maxx, maxy);
    return b;
}

void mapnik_bbox_free(mapnik_bbox_t * b) {
    if (b)
        delete b;
}

void mapnik_map_zoom_to_box(mapnik_map_t * m, mapnik_bbox_t * b) {
    if (m && m->m && b) {
        m->m->zoom_to_box(b->b);
    }
}

struct _mapnik_image_t {
    mapnik_image_type *i;
};

void mapnik_image_free(mapnik_image_t * i) {
    if (i) {
        if (i->i) delete i->i;
        delete i;
    }
}

mapnik_image_t * mapnik_map_render_to_image(mapnik_map_t * m) {
    ensure_readers_registered();
    mapnik_map_reset_last_error(m);
    mapnik_image_type * im = NULL;
    if (m && m->m) {
        im = new mapnik_image_type(m->m->width(), m->m->height());
        try {
            mapnik::agg_renderer<mapnik_image_type> ren(*m->m,*im);
            ren.apply();
        } catch (std::exception const& ex) {
            delete im;
            m->err = new std::string(ex.what());
            return NULL;
        }
    }
    mapnik_image_t * i = new mapnik_image_t;
    i->i = im;
    return i;
}

void mapnik_map_add_layer(mapnik_map_t *m, mapnik_layer_t *l) {
    if (m && m->m && l && l->l) {
#if MAPNIK_VERSION >= 300000
        m->m->add_layer(*(l->l));
#else
        m->m->addLayer(*(l->l));
#endif
    }
}

size_t mapnik_map_layer_count(mapnik_map_t *m) {
    if (m && m->m) {
        return m->m->layer_count();
    }
    return 0;
}

mapnik_layer_t * mapnik_map_get_layer(mapnik_map_t *m, size_t i) {
    if (m && m->m && i < m->m->layer_count()) {
        mapnik_layer_t * l = new mapnik_layer_t;
        l->l = &m->m->layers()[i];
        return l;
    }
    return NULL;
}

mapnik_grid_t * mapnik_map_render_to_grid(mapnik_map_t * m, mapnik_layer_t * l, const char * key) {
    ensure_readers_registered();
    mapnik_map_reset_last_error(m);
    mapnik::grid * grid = NULL;
    if (m && m->m && l && l->l) {
        grid = new mapnik::grid(m->m->width(), m->m->height(), key);

        using attributes_type = std::vector<mapnik::attribute_descriptor>;
        mapnik::layer_descriptor ld = l->l->datasource()->get_descriptor();
        attributes_type const& descs = ld.get_descriptors();
        for (attributes_type::const_iterator it = descs.begin(); it != descs.end(); ++it) {
            grid->add_field(it->get_name());
        }

        try {
            mapnik::grid_renderer<mapnik::grid> ren(*m->m,*grid);
            std::set<std::string> fields(grid->get_fields());
            ren.apply(*l->l, fields, 4);
        } catch (std::exception const& ex) {
            delete grid;
            m->err = new std::string(ex.what());
            return NULL;
        }
    }
    mapnik_grid_t * g = new mapnik_grid_t;
    g->g = grid;
    return g;
}

void mapnik_blob_free(mapnik_blob_t * b) {
    if (b) {
        if (b->ptr)
            delete[] b->ptr;
        delete b;
    }
}

mapnik_blob_t * mapnik_image_to_png_blob(mapnik_image_t * i) {
    mapnik_blob_t * blob = new mapnik_blob_t;
    blob->ptr = NULL;
    blob->len = 0;
    if (i && i->i) {
        std::string s = save_to_string(*(i->i), "png256");
        blob->len = s.length();
        blob->ptr = new char[blob->len];
        memcpy(blob->ptr, s.c_str(), blob->len);
    }
    return blob;
}

struct _mapnik_parameters_t {
    mapnik::parameters *p;
};

mapnik_parameters_t *mapnik_parameters() {
    mapnik_parameters_t *params = new mapnik_parameters_t;
    params->p = new mapnik::parameters;
    return params;
}

void mapnik_parameters_free(mapnik_parameters_t *p) {
    if (p) {
        if (p->p) delete p->p;
        delete p;
    }
}

void mapnik_parameters_set(mapnik_parameters_t *p, const char *key, const char *value) {
    if (p && p->p) {
        (*(p->p))[key] = value;
    }
}

struct _mapnik_datasource_t {
    mapnik::datasource_ptr ds;
};

mapnik_datasource_t *mapnik_datasource(mapnik_parameters_t *p) {
    if (p && p->p) {
        mapnik_datasource_t *ds = new mapnik_datasource_t;
#if MAPNIK_VERSION >= 200200
        ds->ds = mapnik::datasource_cache::instance().create(*(p->p));
#else
        ds->ds = mapnik::datasource_cache::instance()->create(*(p->p));
#endif
        return ds;
    }
    return NULL;
}

void mapnik_datasource_free(mapnik_datasource_t *ds) {
    if (ds) {
        delete ds;
    }
}

mapnik_layer_t *mapnik_layer(const char *name, const char *srs) {
    mapnik_layer_t *layer = new mapnik_layer_t;
    layer->l = new mapnik::layer(name, srs);
    return layer;
}

void mapnik_layer_free(mapnik_layer_t *l) {
    if (l)  {
        if (l->l) delete l->l;
        delete l;
    }
}

void mapnik_layer_add_style(mapnik_layer_t *l, const char *stylename) {
    if (l && l->l) {
        l->l->add_style(stylename);
    }
}

void mapnik_layer_set_datasource(mapnik_layer_t *l, mapnik_datasource_t *ds) {
    if (l && l->l && ds && ds->ds) {
        l->l->set_datasource(ds->ds);
    }
}

const char *mapnik_layer_name(mapnik_layer_t *l) {
    if (l && l->l) {
        return l->l->name().c_str();
    }
    return NULL;
}

void mapnik_layer_set_active(mapnik_layer_t *l, int active) {
    if (l && l->l) {
        l->l->set_active(active);
    }
}

const char * mapnik_version_string() {
#if MAPNIK_VERSION >= 200100
    return MAPNIK_VERSION_STRING;
#else
#define MAPNIK_C_API_STRINGIFY(n) #n
    return "ABI " MAPNIK_C_API_STRINGIFY(MAPNIK_VERSION);
#endif
}


#ifdef __cplusplus
}
#endif
