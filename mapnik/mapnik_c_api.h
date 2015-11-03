#ifndef MAPNIK_C_API_H
#define MAPNIK_C_API_H

#if defined(WIN32) || defined(WINDOWS) || defined(_WIN32) || defined(_WINDOWS)
#  define MAPNIKCAPICALL __declspec(dllexport)
#else
#  define MAPNIKCAPICALL __attribute__ ((visibility ("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

MAPNIKCAPICALL int mapnik_register_datasources(const char* path, char** err);
MAPNIKCAPICALL int mapnik_register_fonts(const char* path, char** err);
MAPNIKCAPICALL const char * mapnik_version_string();


// Coord
typedef struct _mapnik_coord_t {
    double x;
    double y;
} mapnik_coord_t;

// Projection
typedef struct _mapnik_projection_t mapnik_projection_t;

MAPNIKCAPICALL void mapnik_projection_free(mapnik_projection_t *p);

MAPNIKCAPICALL mapnik_coord_t mapnik_projection_forward(mapnik_projection_t *p, mapnik_coord_t c);


// Bbox
typedef struct _mapnik_bbox_t mapnik_bbox_t;

MAPNIKCAPICALL mapnik_bbox_t * mapnik_bbox(double minx, double miny, double maxx, double maxy);

MAPNIKCAPICALL void mapnik_bbox_free(mapnik_bbox_t * b);


// Blob
typedef struct _mapnik_blob_t {
    char *ptr;
    unsigned int len;
} mapnik_blob_t;

MAPNIKCAPICALL void mapnik_blob_free(mapnik_blob_t * b);


// Image
typedef struct _mapnik_image_t mapnik_image_t;

MAPNIKCAPICALL void mapnik_image_free(mapnik_image_t * i);

MAPNIKCAPICALL mapnik_blob_t * mapnik_image_to_png_blob(mapnik_image_t * i);


// Parameters
typedef struct _mapnik_parameters_t mapnik_parameters_t;

MAPNIKCAPICALL mapnik_parameters_t *mapnik_parameters();

MAPNIKCAPICALL void mapnik_parameters_free(mapnik_parameters_t *p);

MAPNIKCAPICALL void mapnik_parameters_set(mapnik_parameters_t *p, const char *key, const char *value);


// Datasource
typedef struct _mapnik_datasource_t mapnik_datasource_t;

MAPNIKCAPICALL mapnik_datasource_t *mapnik_datasource(mapnik_parameters_t *p);

MAPNIKCAPICALL void mapnik_datasource_free(mapnik_datasource_t *ds);


// Layer
typedef struct _mapnik_layer_t mapnik_layer_t;

MAPNIKCAPICALL mapnik_layer_t *mapnik_layer(const char *name, const char *srs);

MAPNIKCAPICALL void mapnik_layer_free(mapnik_layer_t *l);

MAPNIKCAPICALL void mapnik_layer_add_style(mapnik_layer_t *l, const char *stylename);

MAPNIKCAPICALL void mapnik_layer_set_datasource(mapnik_layer_t *l, mapnik_datasource_t *ds);

MAPNIKCAPICALL const char *mapnik_layer_name(mapnik_layer_t *l);

MAPNIKCAPICALL void mapnik_layer_set_active(mapnik_layer_t *l, int active);


// Grid
typedef struct _mapnik_grid_t mapnik_grid_t;

MAPNIKCAPICALL void mapnik_grid_free(mapnik_grid_t * g);

MAPNIKCAPICALL char * mapnik_grid_to_json(mapnik_grid_t * g, unsigned res);


//  Map
typedef struct _mapnik_map_t mapnik_map_t;

MAPNIKCAPICALL mapnik_map_t * mapnik_map( unsigned int width, unsigned int height );

MAPNIKCAPICALL void mapnik_map_free(mapnik_map_t * m);

MAPNIKCAPICALL const char * mapnik_map_last_error(mapnik_map_t * m);

MAPNIKCAPICALL const char * mapnik_map_get_srs(mapnik_map_t * m);

MAPNIKCAPICALL int mapnik_map_set_srs(mapnik_map_t * m, const char* srs);

MAPNIKCAPICALL int mapnik_map_load(mapnik_map_t * m, const char* stylesheet);

MAPNIKCAPICALL int mapnik_map_load_string(mapnik_map_t * m, const char* stylesheet_string);

MAPNIKCAPICALL int mapnik_map_zoom_all(mapnik_map_t * m);

MAPNIKCAPICALL int mapnik_map_render_to_file(mapnik_map_t * m, const char* filepath);

MAPNIKCAPICALL void mapnik_map_resize(mapnik_map_t * m, unsigned int width, unsigned int height);

MAPNIKCAPICALL void mapnik_map_set_buffer_size(mapnik_map_t * m, int buffer_size);

MAPNIKCAPICALL void mapnik_map_zoom_to_box(mapnik_map_t * m, mapnik_bbox_t * b);

MAPNIKCAPICALL mapnik_projection_t * mapnik_map_projection(mapnik_map_t *m);

MAPNIKCAPICALL mapnik_image_t * mapnik_map_render_to_image(mapnik_map_t * m);

MAPNIKCAPICALL void mapnik_map_add_layer(mapnik_map_t *m, mapnik_layer_t *l);

MAPNIKCAPICALL size_t mapnik_map_layer_count(mapnik_map_t *m);

MAPNIKCAPICALL mapnik_layer_t * mapnik_map_get_layer(mapnik_map_t *m, size_t i);

MAPNIKCAPICALL mapnik_grid_t * mapnik_map_render_to_grid(mapnik_map_t * m, mapnik_layer_t * l, const char * key);


#ifdef __cplusplus
}
#endif


#endif // MAPNIK_C_API_H

