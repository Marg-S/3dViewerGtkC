#include "3dviewer.h"

void save_settings(const Settings *settings) {
  char settings_path[PATH_MAX];
  get_settings_path(settings_path);

  FILE *file = fopen(settings_path, "w");

  if (file == NULL) {
    fprintf(stderr, "Error saving configuration file\n");
  } else {
    fprintf(file, "Projection=%d\n", settings->projection);

    fprintf(file, "EdgeType=%d\n", settings->edge_type);
    fprintf(file, "EdgeColor=%f %f %f %f\n", settings->edge_color.red,
            settings->edge_color.green, settings->edge_color.blue,
            settings->edge_color.alpha);
    fprintf(file, "EdgeThickness=%f\n", settings->edge_thickness);
    fprintf(file, "EdgeDisplayMethod=%d\n", settings->edge_display_method);

    fprintf(file, "VertexColor=%f %f %f %f\n", settings->vertex_color.red,
            settings->vertex_color.green, settings->vertex_color.blue,
            settings->vertex_color.alpha);
    fprintf(file, "VertexSize=%f\n", settings->vertex_size);

    fprintf(file, "BackgroundColor=%f %f %f %f\n",
            settings->background_color.red, settings->background_color.green,
            settings->background_color.blue, settings->background_color.alpha);

    fclose(file);
  }
}

void load_settings(Settings *settings) {
  set_default_settings(settings);

  char settings_path[PATH_MAX];
  get_settings_path(settings_path);

  FILE *file = fopen(settings_path, "r");

  if (file == NULL) {
    fprintf(stderr, "Error loading configuration file\n");
  } else {
    double r = 0.0, g = 0.0, b = 0.0, a = 0.0;

    fscanf(file, "Projection=%d\n", (int *)&settings->projection);
    fscanf(file, "EdgeType=%d\n", (int *)&settings->edge_type);
    fscanf(file, "EdgeColor=%lf %lf %lf %lf\n", &r, &g, &b, &a);
    settings->edge_color.red = r;
    settings->edge_color.green = g;
    settings->edge_color.blue = b;
    settings->edge_color.alpha = a;

    fscanf(file, "EdgeThickness=%lf\n", &settings->edge_thickness);
    fscanf(file, "EdgeDisplayMethod=%d\n",
           (int *)&settings->edge_display_method);

    fscanf(file, "VertexColor=%lf %lf %lf %lf\n", &r, &g, &b, &a);
    settings->vertex_color.red = r;
    settings->vertex_color.green = g;
    settings->vertex_color.blue = b;
    settings->vertex_color.alpha = a;

    fscanf(file, "VertexSize=%lf\n", &settings->vertex_size);

    fscanf(file, "BackgroundColor=%lf %lf %lf %lf\n", &r, &g, &b, &a);
    settings->background_color.red = r;
    settings->background_color.green = g;
    settings->background_color.blue = b;
    settings->background_color.alpha = a;

    fclose(file);
  }
}

void set_default_settings(Settings *settings) {
  settings->projection = PARALLEL_PROJECTION;
  settings->edge_type = SOLID_EDGE;
  settings->edge_display_method = NONE_EDGE;
  settings->edge_color.red = 1.0;
  settings->edge_color.green = 1.0;
  settings->edge_color.blue = 1.0;
  settings->edge_color.alpha = 1.0;
  settings->edge_thickness = 1.0;
  settings->vertex_color.red = 0.0;
  settings->vertex_color.green = 1.0;
  settings->vertex_color.blue = 0.0;
  settings->vertex_color.alpha = 1.0;
  settings->vertex_size = 5.0;
  settings->background_color.red = 0.0;
  settings->background_color.green = 0.0;
  settings->background_color.blue = 0.0;
  settings->background_color.alpha = 1.0;
}

int get_settings_path(char *settings_path) {
  int error_code = OK;
  const char *dir = "build";

  if (dir) {
    sprintf(settings_path, "%s/%s", dir, SETTINGS_CONFIG);
  } else {
    fprintf(stderr, "Error getting home directory\n");
    error_code = ERROR;
  }

  return error_code;
}