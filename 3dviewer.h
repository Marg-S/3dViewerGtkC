#ifndef VIEWER
#define VIEWER

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ERROR_CODES { OK, ERROR };
#define PI 3.14159265358979323846264338327950288
#define MAX_LINE_LENGTH 2048
#define SETTINGS_CONFIG "settings.conf"

// Structures
typedef struct model1 {
  unsigned int vertex_count;
  double *vertices;
  unsigned int face_count;
  unsigned int *faces;
  unsigned int polygon_count;
  int *num_vertices_in_polygon;
  double minMaxX[2];
  double minMaxY[2];
  double minMaxZ[2];
} Model1;

typedef struct {
  double data[4][4];
} Matrix;

// Settings
typedef enum { PARALLEL_PROJECTION, CENTRAL_PROJECTION } ProjectionType;

typedef enum { SOLID_EDGE, DASHED_EDGE } EdgeType;

typedef enum { NONE_EDGE, CIRCLE_EDGE, SQUARE_EDGE } EdgeDisplayMethod;

typedef struct color_rgba {
  float red;
  float green;
  float blue;
  float alpha;
} ColorRGBA;

typedef struct settings {
  ProjectionType projection;
  EdgeType edge_type;
  EdgeDisplayMethod edge_display_method;
  ColorRGBA edge_color;
  double edge_thickness;
  ColorRGBA vertex_color;
  double vertex_size;
  ColorRGBA background_color;
} Settings;

// Parser
void *memory_allocation(size_t size, const char *error_msg);
void replace_decimal_separator(char *str);
int load_model(const char *filename, Model1 *model);
int get_model_data(FILE *file, Model1 *model);
void free_model(Model1 *model);
void read_line(FILE *file, char **line);
void replace_decimal_separator(char *str);
int parse_file(FILE *file, char *line, unsigned int *capability, Model1 *model);
int parse_vertices(char *line, unsigned int *vertex_index, Model1 *model);
int parse_faces(char *line, unsigned int *capability, Model1 *model,
                unsigned int *face_index, unsigned int *polygon_index);
int count_vertices_faces(char *line, FILE *file, unsigned int *vertex_count,
                         unsigned int *face_count);

// Transfotmation
double convert_to_radian(double angle);
Matrix create_identity_matrix();
void mult_matrix(Matrix matrix, double vector[4], double result[4]);
Matrix create_translation_matrix(double x, double y, double z);
Matrix create_rotation_matrix_x(double angle);
Matrix create_rotation_matrix_y(double angle);
Matrix create_rotation_matrix_z(double angle);
Matrix create_scale_matrix(double scale);
void modify_model(Model1 *model, Matrix matrix);
void translate_to_origin(Model1 *model);
void scale1(Model1 *model);

// Settings
void save_settings(const Settings *settings);
void load_settings(Settings *settings);
void set_default_settings(Settings *settings);
int get_settings_path(char *settings_path);

#endif  // VIEWER
