#include "3dviewer.h"

#include <epoxy/gl.h>
#include <gtk/gtk.h>

static void realize(GtkWidget *gl_area, gpointer data) {
  gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
  if (gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) != NULL) return;

  glEnable(GL_DEPTH_CLAMP);

  const char *vertex_shader_source =
      "#version 330 core\n"
      "layout(location = 0) in vec3 position;\n"
      "void main() { gl_Position = vec4(position.x, position.y, position.z, "
      "1.0); }\0";
  GLuint vertex_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  const char *fragment_shader_source =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "uniform vec4 vertexColor;"
      "void main() { FragColor = vertexColor; }\0";
  GLuint fragment_shader;
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  unsigned int shader_program;
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLuint vao, vbo, ebo;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  g_object_set_data(G_OBJECT(gl_area), "vao", GUINT_TO_POINTER(vao));
  g_object_set_data(G_OBJECT(gl_area), "vbo", GUINT_TO_POINTER(vbo));
  g_object_set_data(G_OBJECT(gl_area), "ebo", GUINT_TO_POINTER(ebo));
  g_object_set_data(G_OBJECT(gl_area), "shader-program",
                    GUINT_TO_POINTER(shader_program));
  glBindVertexArray(0);
}

static void unrealize(GtkWidget *gl_area) {
  gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
  if (gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) != NULL) return;

  Settings *settings = g_object_get_data(G_OBJECT(gl_area), "settings");
  save_settings(settings);

  unsigned int vao =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "vao"));
  unsigned int vbo =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "vbo"));
  unsigned int ebo =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "ebo"));
  unsigned int shader_program =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "shader-program"));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &ebo);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteProgram(shader_program);
  Model1 *model = g_object_get_data(G_OBJECT(gl_area), "model");
  free_model(model);
}

static void clicked(GtkWidget *button, gpointer gl_area) {
  GtkSpinButton *spin =
      GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "x"));
  double x = gtk_spin_button_get_value(spin);

  spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "y"));
  double y = gtk_spin_button_get_value(spin);

  spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "z"));
  double z = gtk_spin_button_get_value(spin);

  Model1 *model = (g_object_get_data(G_OBJECT(gl_area), "model"));
  Matrix matrix;

  const char *name = gtk_button_get_label(GTK_BUTTON(button));
  if (strstr(name, "Move") && (x || y || z) != 0) {
    Matrix matrix = create_translation_matrix(x, y, z);
    modify_model(model, matrix);
  } else {
    if (x) {
      matrix = create_rotation_matrix_x(x);
      modify_model(model, matrix);
    }
    if (y) {
      matrix = create_rotation_matrix_y(y);
      modify_model(model, matrix);
    }
    if (z) {
      matrix = create_rotation_matrix_z(z);
      modify_model(model, matrix);
    }
  }

  gtk_widget_queue_draw(GTK_WIDGET(gl_area));
}

static void clicked_scale(GtkWidget *button, gpointer gl_area) {
  GtkSpinButton *spin_scale =
      GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "spin-scale"));
  double x = gtk_spin_button_get_value(spin_scale);

  if (x) {
    Model1 *model = (g_object_get_data(G_OBJECT(gl_area), "model"));
    Matrix matrix;
    matrix = create_scale_matrix(x);
    modify_model(model, matrix);
  }

  gtk_widget_queue_draw(GTK_WIDGET(gl_area));
}

void draw(GtkWidget *gl_area, GdkGLContext *context, Settings *settings,
          Model1 *model) {
  glEnableVertexAttribArray(0);
  unsigned int shader_program =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "shader-program"));
  glUseProgram(shader_program);

  GLint vertex_color_location =
      glGetUniformLocation(shader_program, "vertexColor");
  ColorRGBA *color = &(settings->edge_color);
  glUniform4f(vertex_color_location, color->red, color->green, color->blue,
              color->alpha);

  for (int i = 0, offset = 0; i < model->polygon_count; i++) {
    int count = model->num_vertices_in_polygon[i];
    void *ptr = (void *)(offset * sizeof(unsigned int));
    glDrawElementsBaseVertex(GL_LINE_LOOP, count, GL_UNSIGNED_INT, ptr, -1);
    offset += model->num_vertices_in_polygon[i];
  }
  if (settings->edge_display_method != NONE_EDGE) {
    if (settings->edge_display_method == CIRCLE_EDGE)
      glEnable(GL_POINT_SMOOTH);
    else
      glDisable(GL_POINT_SMOOTH);
    glPointSize(settings->vertex_size);
    color = &(settings->vertex_color);
    glUniform4f(vertex_color_location, color->red, color->green, color->blue,
                color->alpha);
    glDrawArrays(GL_POINTS, 0, model->vertex_count);
  }
  glDisableVertexAttribArray(0);
}

static gboolean render(GtkWidget *gl_area, GdkGLContext *context) {
  Settings *settings = g_object_get_data(G_OBJECT(gl_area), "settings");
  ColorRGBA *color = &(settings->background_color);
  glClearColor(color->red, color->green, color->blue, color->alpha);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Model1 *model = g_object_get_data(G_OBJECT(gl_area), "model");
  if (model->vertex_count != 0) {
    unsigned int vao =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "vao"));
    glBindVertexArray(vao);

    unsigned int size = model->vertex_count * 3 * sizeof(model->vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, size, model->vertices, GL_DYNAMIC_DRAW);

    glLineWidth(settings->edge_thickness);

    if (settings->edge_type == DASHED_EDGE) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(3, 0x00FF);
    } else
      glDisable(GL_LINE_STIPPLE);

    if (settings->projection == PARALLEL_PROJECTION) {
      glOrtho(0.1, 1., 0.1, 1., 0.1, 100.);
    } else {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-1., 1., -1., 1., 1., 100.);
      glMatrixMode(GL_MODELVIEW);
    }

    draw(gl_area, context, settings, model);
    glBindVertexArray(0);
  }

  glFlush();
  return TRUE;
}

void load_buffer(GtkWidget *gl_area) {
  gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
  if (gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) != NULL) return;

  unsigned int vao =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "vao"));
  glBindVertexArray(vao);

  Model1 *model = g_object_get_data(G_OBJECT(gl_area), "model");

  unsigned int vbo =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "vbo"));
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  unsigned int size = model->vertex_count * 3 * sizeof(model->vertices[0]);
  glBufferData(GL_ARRAY_BUFFER, size, model->vertices, GL_STATIC_DRAW);

  unsigned int ebo =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gl_area), "ebo"));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  size = model->face_count * sizeof(model->faces[0]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, model->faces, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_DOUBLE, GL_TRUE, 3 * sizeof(double),
                        (void *)0);

  gtk_widget_queue_draw(gl_area);
}

static void open_dialog_response(GtkNativeDialog *dialog, int response,
                                 GObject *gl_area) {
  gtk_native_dialog_hide(dialog);

  if (response == GTK_RESPONSE_ACCEPT) {
    GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    const char *filename = g_file_peek_path(file);
    g_object_unref(file);

    Model1 *model = g_object_get_data(gl_area, "model");
    free_model(model);

    if (load_model(filename, model) == OK) {
      GtkLabel *status = g_object_get_data(gl_area, "status");
      char str_status[256];
      sprintf(str_status, "File: %s (%d vertices, %d edges)", filename,
              model->vertex_count, model->polygon_count);
      gtk_label_set_label(status, str_status);

      translate_to_origin(model);
      scale1(model);

      load_buffer(GTK_WIDGET(gl_area));
    } else
      free_model(model);
  }

  gtk_native_dialog_destroy(dialog);
}

static void clicked_open(GtkWidget *button, GObject *gl_area) {
  GtkFileChooserNative *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_native_new(
      "Select an object",
      GTK_WINDOW(gtk_widget_get_ancestor(button, GTK_TYPE_WINDOW)),
      GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");

  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_filter_set_name(filter, "All Files");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  g_object_unref(filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_add_suffix(filter, "obj");
  gtk_file_filter_set_name(filter, "Objects");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  g_object_unref(filter);

  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
  g_object_unref(filter);

  gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
  g_signal_connect(dialog, "response", G_CALLBACK(open_dialog_response),
                   gl_area);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static void color_set(GtkColorButton *button, GObject *gl_area) {
  Settings *settings = g_object_get_data(G_OBJECT(gl_area), "settings");
  const char *name = gtk_widget_get_name(GTK_WIDGET(button));
  ColorRGBA *color;
  if (strstr(name, "bg")) color = &(settings->background_color);
  if (strstr(name, "edge")) color = &(settings->edge_color);
  if (strstr(name, "point")) color = &(settings->vertex_color);

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), (GdkRGBA *)color);
  gtk_widget_queue_draw(GTK_WIDGET(gl_area));
}

static void check_toggled(GtkCheckButton *check_btn, GObject *gl_area) {
  Settings *settings = g_object_get_data(G_OBJECT(gl_area), "settings");
  const char *label = gtk_check_button_get_label(check_btn);

  if (strstr(label, "Parallel"))
    settings->projection = !gtk_check_button_get_active(check_btn);

  if (strstr(label, "Dashed"))
    settings->edge_type = gtk_check_button_get_active(check_btn);

  if (gtk_check_button_get_active(check_btn)) {
    if (strstr(label, "None")) settings->edge_display_method = NONE_EDGE;
    if (strstr(label, "Circle")) settings->edge_display_method = CIRCLE_EDGE;
    if (strstr(label, "Square")) settings->edge_display_method = SQUARE_EDGE;
  }

  gtk_widget_queue_draw(GTK_WIDGET(gl_area));
}

static void changed(GtkSpinButton *spin, GObject *gl_area) {
  Settings *settings = g_object_get_data(G_OBJECT(gl_area), "settings");
  float size = gtk_spin_button_get_value(spin);
  const char *name = gtk_widget_get_name(GTK_WIDGET(spin));

  if (strstr(name, "point"))
    settings->vertex_size = size;
  else
    settings->edge_thickness = size;

  gtk_widget_queue_draw(GTK_WIDGET(gl_area));
}

void set_settings(GtkBuilder *builder, Settings *settings, GObject *gl_area) {
  GObject *radio_parallel = gtk_builder_get_object(builder, "check-parallel");
  GObject *radio_central = gtk_builder_get_object(builder, "check-central");
  g_signal_connect(radio_parallel, "toggled", G_CALLBACK(check_toggled),
                   gl_area);
  if (settings->projection == PARALLEL_PROJECTION)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_parallel), 1);
  else
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_central), 1);

  GObject *check_dashed = gtk_builder_get_object(builder, "check-dashed");
  g_signal_connect(check_dashed, "toggled", G_CALLBACK(check_toggled), gl_area);
  gboolean dashed = 0;
  if (settings->edge_type == DASHED_EDGE) dashed = 1;
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_dashed), dashed);

  GObject *check_none = gtk_builder_get_object(builder, "check-none");
  GObject *check_circle = gtk_builder_get_object(builder, "check-circle");
  GObject *check_square = gtk_builder_get_object(builder, "check-square");
  g_signal_connect(check_none, "toggled", G_CALLBACK(check_toggled), gl_area);
  g_signal_connect(check_circle, "toggled", G_CALLBACK(check_toggled), gl_area);
  g_signal_connect(check_square, "toggled", G_CALLBACK(check_toggled), gl_area);
  if (settings->edge_display_method == NONE_EDGE)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_none), 1);
  if (settings->edge_display_method == CIRCLE_EDGE)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_circle), 1);
  if (settings->edge_display_method == SQUARE_EDGE)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_square), 1);

  GObject *button_color = gtk_builder_get_object(builder, "button-color-bg");
  g_signal_connect(button_color, "color-set", G_CALLBACK(color_set), gl_area);
  const GdkRGBA *color = (const GdkRGBA *)&(settings->background_color);
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button_color), color);
  button_color = gtk_builder_get_object(builder, "button-color-edge");
  g_signal_connect(button_color, "color-set", G_CALLBACK(color_set), gl_area);
  color = (const GdkRGBA *)&(settings->edge_color);
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button_color), color);
  button_color = gtk_builder_get_object(builder, "button-color-point");
  g_signal_connect(button_color, "color-set", G_CALLBACK(color_set), gl_area);
  color = (const GdkRGBA *)&(settings->vertex_color);
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button_color), color);

  GObject *spin_point = gtk_builder_get_object(builder, "spin-point");
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_point), settings->vertex_size);
  g_signal_connect(spin_point, "value-changed", G_CALLBACK(changed), gl_area);
  GObject *spin_edge = gtk_builder_get_object(builder, "spin-edge");
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_edge),
                            settings->edge_thickness);
  g_signal_connect(spin_edge, "value-changed", G_CALLBACK(changed), gl_area);
}

static void activate(GtkApplication *app) {
  GtkBuilder *builder = gtk_builder_new_from_file("3dviewer_view.ui");
  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  gtk_window_set_application(GTK_WINDOW(window), app);

  GObject *gl_area = gtk_builder_get_object(builder, "gl-area");
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

  static Settings settings = {0};
  load_settings(&settings);
  g_object_set_data(gl_area, "settings", &settings);

  static Model1 model = {0};
  g_object_set_data(gl_area, "model", &model);

  GObject *button_open = gtk_builder_get_object(builder, "button-open");
  g_signal_connect(button_open, "clicked", G_CALLBACK(clicked_open), gl_area);

  GObject *button_move = gtk_builder_get_object(builder, "button-move");
  g_signal_connect(button_move, "clicked", G_CALLBACK(clicked), gl_area);
  GObject *spin_move = gtk_builder_get_object(builder, "spin-move-x");
  g_object_set_data(button_move, "x", spin_move);
  spin_move = gtk_builder_get_object(builder, "spin-move-y");
  g_object_set_data(button_move, "y", spin_move);
  spin_move = gtk_builder_get_object(builder, "spin-move-z");
  g_object_set_data(button_move, "z", spin_move);

  GObject *button_rotate = gtk_builder_get_object(builder, "button-rotate");
  g_signal_connect(button_rotate, "clicked", G_CALLBACK(clicked), gl_area);
  GObject *spin_rotate = gtk_builder_get_object(builder, "spin-rotate-x");
  g_object_set_data(button_rotate, "x", spin_rotate);
  spin_rotate = gtk_builder_get_object(builder, "spin-rotate-y");
  g_object_set_data(button_rotate, "y", spin_rotate);
  spin_rotate = gtk_builder_get_object(builder, "spin-rotate-z");
  g_object_set_data(button_rotate, "z", spin_rotate);

  GObject *button_scale = gtk_builder_get_object(builder, "button-scale");
  g_signal_connect(button_scale, "clicked", G_CALLBACK(clicked_scale), gl_area);
  GObject *spin_scale = gtk_builder_get_object(builder, "spin-scale");
  g_object_set_data(button_scale, "spin-scale", spin_scale);

  GObject *status = gtk_builder_get_object(builder, "status");
  g_object_set_data(gl_area, "status", status);

  set_settings(builder, &settings, gl_area);
  gtk_window_present(GTK_WINDOW(window));
  g_object_unref(builder);
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("my.viewer.c", 0);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);

  return status;
}
