#include "3dviewer.h"

void read_line(FILE *file, char **line) {
  if (fgets(*line, MAX_LINE_LENGTH, file) != NULL) {
    size_t line_length = strlen(*line);
    if (line_length > 0 && (*line)[line_length - 1] == '\n') {
      (*line)[line_length - 1] = '\0';
    }
  } else {
    *line = NULL;
  }
}

void *memory_allocation(size_t size, const char *error_msg) {
  void *ptr = malloc(size);

  if (ptr == NULL) {
    fprintf(stderr, "Ошибка выделения памяти: %s\n", error_msg);
  }

  return ptr;
}

double convert_to_radian(double angle) { return PI / 180 * angle; }

Matrix create_identity_matrix() {
  Matrix matrix;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        matrix.data[i][j] = 1;
      } else {
        matrix.data[i][j] = 0;
      }
    }
  }
  return matrix;
}

void mult_matrix(Matrix matrix, double vector[4], double result[4]) {
  for (int i = 0; i < 4; i++) {
    double sum = 0;
    for (int j = 0; j < 4; j++) {
      sum += matrix.data[i][j] * vector[j];
    }
    result[i] = sum;
  }
}

Matrix create_translation_matrix(double x, double y, double z) {
  Matrix matrix = create_identity_matrix();

  matrix.data[0][3] = x;
  matrix.data[1][3] = y;
  matrix.data[2][3] = z;

  return matrix;
}

Matrix create_rotation_matrix_x(double angle) {
  double radian = convert_to_radian(angle);
  Matrix matrix = create_identity_matrix();

  double cos_value = cos(radian);
  double sin_value = sin(radian);

  matrix.data[1][1] = cos_value;
  matrix.data[1][2] = -sin_value;
  matrix.data[2][1] = sin_value;
  matrix.data[2][2] = cos_value;

  return matrix;
}

Matrix create_rotation_matrix_y(double angle) {
  double radian = convert_to_radian(angle);
  Matrix matrix = create_identity_matrix();

  double cos_value = cos(radian);
  double sin_value = sin(radian);

  matrix.data[0][0] = cos_value;
  matrix.data[0][2] = sin_value;
  matrix.data[2][0] = -sin_value;
  matrix.data[2][2] = cos_value;

  return matrix;
}

Matrix create_rotation_matrix_z(double angle) {
  double radian = convert_to_radian(angle);
  Matrix matrix = create_identity_matrix();

  double cos_value = cos(radian);
  double sin_value = sin(radian);

  matrix.data[0][0] = cos_value;
  matrix.data[0][1] = -sin_value;
  matrix.data[1][0] = sin_value;
  matrix.data[1][1] = cos_value;

  return matrix;
}

Matrix create_scale_matrix(double scale) {
  Matrix matrix = create_identity_matrix();

  matrix.data[0][0] = scale;
  matrix.data[1][1] = scale;
  matrix.data[2][2] = scale;

  return matrix;
}

void modify_model(Model1 *model, Matrix matrix) {
  for (unsigned int i = 0; i < model->vertex_count * 3; i += 3) {
    double vector[4] = {model->vertices[i], model->vertices[i + 1],
                        model->vertices[i + 2], 1.0};

    double result[4] = {0};
    mult_matrix(matrix, vector, result);

    model->vertices[i] = result[0];
    model->vertices[i + 1] = result[1];
    model->vertices[i + 2] = result[2];
  }
}

void translate_to_origin(Model1 *model) {
  double centerX = (model->minMaxX[0] + model->minMaxX[1]) / 2;
  double centerY = (model->minMaxY[0] + model->minMaxY[1]) / 2;
  double centerZ = (model->minMaxZ[0] + model->minMaxZ[1]) / 2;

  Matrix matrix = create_translation_matrix(-centerX, -centerY, -centerZ);
  modify_model(model, matrix);
}

void scale1(Model1 *model) {
  double x = model->minMaxX[1] - model->minMaxX[0];
  double y = model->minMaxY[1] - model->minMaxY[0];
  double z = model->minMaxZ[1] - model->minMaxZ[0];

  double max = y;
  if (x > max) {
    max = x;
  } else if (z > max) {
    max = z;
  }

  double scale_value = 0.5;
  double scale = (scale_value - (scale_value * (-1))) / max;

  Matrix matrix = create_scale_matrix(scale);
  modify_model(model, matrix);
}

int load_model(const char *filename, Model1 *model) {
  int error_code = OK;
  setlocale(LC_NUMERIC, "en_US.UTF-8");
  FILE *file = fopen(filename, "r");

  if (file == NULL) {
    fprintf(stderr, "Ошибка при открытии файла: %s\n", filename);
    error_code = ERROR;
  } else {
    error_code = get_model_data(file, model);
  }

  return error_code;
}

int get_model_data(FILE *file, Model1 *model) {
  int error_code = OK;
  unsigned int vertex_count = 0;
  unsigned int face_count = 0;
  char line[MAX_LINE_LENGTH];

  error_code = count_vertices_faces(line, file, &vertex_count, &face_count);

  if (error_code == OK) {
    model->vertex_count = vertex_count;
    model->polygon_count = face_count;
  }

  if (error_code == OK) {
    model->vertices = (double *)memory_allocation(
        sizeof(double) * 3 * vertex_count, "model.vertices");

    if (!model->vertices) {
      error_code = ERROR;
    }
  }

  if (error_code == OK) {
    model->num_vertices_in_polygon = (int *)memory_allocation(
        sizeof(int) * face_count, "model.num_vertices_in_polygon");

    if (!model->num_vertices_in_polygon) {
      error_code = ERROR;
    }
  }

  unsigned int capability = 3 * face_count;

  if (error_code == OK) {
    model->faces = (unsigned int *)memory_allocation(
        sizeof(unsigned int) * capability, "model.faces");

    if (!model->faces) {
      error_code = ERROR;
    }
  }

  model->minMaxX[0] = DBL_MAX;
  model->minMaxX[1] = -DBL_MAX;
  model->minMaxY[0] = DBL_MAX;
  model->minMaxY[1] = -DBL_MAX;
  model->minMaxZ[0] = DBL_MAX;
  model->minMaxZ[1] = -DBL_MAX;

  if (error_code == OK) {
    fseek(file, 0, SEEK_SET);
    error_code = parse_file(file, line, &capability, model);
  }

  fclose(file);
  return error_code;
}

void free_model(Model1 *model) {
  if (model->vertices) {
    free(model->vertices);
  }
  if (model->faces) {
    free(model->faces);
  }
  if (model->num_vertices_in_polygon) {
    free(model->num_vertices_in_polygon);
  }
  memset(model, 0, sizeof(*model));
}

void replace_decimal_separator(char *str) {
  for (char *ptr = str; *ptr != '\0'; ptr++) {
    if (*ptr == ',') {
      *ptr = '.';
    }
  }
}

int parse_vertices(char *line, unsigned int *vertex_index, Model1 *model) {
  int error_code = OK;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  replace_decimal_separator(line);
  int parsed = sscanf(line, "v %lf %lf %lf", &x, &y, &z);

  if (parsed != 3) {
    fprintf(stderr, "Parsing error\n");
    error_code = ERROR;
  } else {
    if (x < model->minMaxX[0]) model->minMaxX[0] = x;
    if (x > model->minMaxX[1]) model->minMaxX[1] = x;
    if (y < model->minMaxY[0]) model->minMaxY[0] = y;
    if (y > model->minMaxY[1]) model->minMaxY[1] = y;
    if (z < model->minMaxZ[0]) model->minMaxZ[0] = z;
    if (z > model->minMaxZ[1]) model->minMaxZ[1] = z;

    model->vertices[*vertex_index] = x;
    model->vertices[*vertex_index + 1] = y;
    model->vertices[*vertex_index + 2] = z;
    *vertex_index += 3;
  }

  return error_code;
}

int parse_faces(char *line, unsigned int *capability, Model1 *model,
                unsigned int *face_index, unsigned int *polygon_index) {
  int error_code = OK;
  int vertex_num = 0;
  unsigned int vertex_start = *face_index;
  char *token;

  token = strtok(line + 1, " \t");

  while (token && error_code == OK) {
    if (*capability < *face_index + 1) {
      *capability = *face_index + 1000;
      model->faces = (unsigned int *)realloc(
          model->faces, sizeof(unsigned int) * (*capability));

      if (model->faces == NULL) {
        fprintf(stderr, "Memory allocation failed while processing face.\n");
        error_code = ERROR;
      }
    }
    if (error_code == OK) {
      sscanf(token, "%d", &vertex_num);
      if (strlen(token) > 0 && sscanf(token, "%d", &vertex_num) == 1) {
        if (vertex_num < 0) {
          fprintf(stderr, "Incorrect .obj file\n");
          error_code = ERROR;
        }
        model->faces[*face_index] = vertex_num;
        *face_index += 1;
      }

      token = strtok(NULL, " \t");
    }
  }
  if (error_code == OK) {
    model->num_vertices_in_polygon[*polygon_index] = *face_index - vertex_start;
    *polygon_index += 1;
  }

  return error_code;
}

int count_vertices_faces(char *line, FILE *file, unsigned int *vertex_count,
                         unsigned int *face_count) {
  int error_code = OK;
  read_line(file, &line);
  while (line != NULL && error_code == OK) {
    if (line[0] == 'v' && line[1] == ' ') {
      *vertex_count += 1;
    } else if (line[0] == 'f' && line[1] == ' ') {
      *face_count += 1;
    }

    read_line(file, &line);
  }

  return error_code;
}

int parse_file(FILE *file, char *line, unsigned int *capability,
               Model1 *model) {
  int error_code = OK;
  unsigned int vertex_index = 0;
  unsigned int face_index = 0;
  unsigned int polygon_index = 0;
  read_line(file, &line);

  while (line && error_code == OK) {
    if (line[0] == 'v' && line[1] == ' ') {
      error_code = parse_vertices(line, &vertex_index, model);
    } else if (line[0] == 'f' && line[1] == ' ') {
      error_code =
          parse_faces(line, capability, model, &face_index, &polygon_index);
    }
    if (error_code == OK) {
      read_line(file, &line);
    }
  }

  if (error_code == OK) {
    model->face_count = face_index;
  }

  return error_code;
}