#include <iostream>
#include <memory>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "file_util.h"
#include "Texture.h"
#include "Exceptions.h"
#include <GLUT/glut.h>
#include <glm/glm/mat4x4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

/* Types */

// These function pointer types are defined by GLEW, which we don't use on Mac.
typedef std::function<void(GLuint, GLsizei, GLsizei *, GLchar *)> \
    PFNGLGETSHADERINFOLOGPROC;
typedef std::function<void(GLuint, GLenum, GLint *)> PFNGLGETSHADERIVPROC;

struct {
  GLuint vertex_buffer;
  GLuint element_buffer;
  std::shared_ptr<dg::Texture> textures[2];

  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint program;

  struct {
    GLuint elapsed_time;
    GLuint textures[2];
    GLuint MATRIX_MVP;
  } uniforms;

  struct {
    GLint position;
  } attributes;

  GLfloat elapsed_time;
} g_resources;

/* Scene data */

static const GLfloat g_vertex_buffer_data[] = { 
  -1.0f, -1.0f, 0.0f, 1.0f,
  1.0f, -1.0f, 0.0f, 1.0f,
  -1.0f,  1.0f, 0.0f, 1.0f,
  1.0f,  1.0f, 0.0f, 1.0f,
};

static const GLushort g_element_buffer_data[] = {
  0, 1, 2, 3,
};

/* Functions */

bool make_resources();
void idle();
void render();
GLuint make_buffer(GLenum target, const void *buffer_data, GLsizei buffer_size);
GLuint make_shader(GLenum type, std::string path);
GLuint make_program(GLuint vertex_shader, GLuint fragment_shader);


int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(400, 300);
  glutCreateWindow("OpenGL Image Fader");
  glutDisplayFunc(&render);
  glutIdleFunc(&idle);

  if (!make_resources()) {
    std::cerr << "Failed to load resources." << std::endl;
    return 1;
  }

  glutMainLoop();

  return 0;
}

bool make_resources() {
  // Make buffers
  g_resources.vertex_buffer = make_buffer(
      GL_ARRAY_BUFFER,
      g_vertex_buffer_data,
      sizeof(g_vertex_buffer_data)
      );
  g_resources.element_buffer = make_buffer(
      GL_ELEMENT_ARRAY_BUFFER,
      g_element_buffer_data,
      sizeof(g_element_buffer_data)
      );

  // Make textures
  try {
    g_resources.textures[0] = std::make_shared<dg::Texture>(
        dg::Texture::FromPath("assets/textures/image1.tga"));
    g_resources.textures[1] = std::make_shared<dg::Texture>(
        dg::Texture::FromPath("assets/textures/image2.tga"));
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }

  // Make shaders

  g_resources.vertex_shader = make_shader(
      GL_VERTEX_SHADER, "assets/shaders/frustum-rotation.v.glsl");
  if (g_resources.vertex_shader == 0) return false;
  g_resources.fragment_shader = make_shader(
      GL_FRAGMENT_SHADER, "assets/shaders/hello-gl.f.glsl");
  if (g_resources.fragment_shader == 0) return false;
  g_resources.program = make_program(
      g_resources.vertex_shader, g_resources.fragment_shader);
  if (g_resources.program == 0) return false;

  // Get handles to shader variables

  g_resources.uniforms.MATRIX_MVP =
    glGetUniformLocation(g_resources.program, "MATRIX_MVP");
  g_resources.uniforms.elapsed_time =
    glGetUniformLocation(g_resources.program, "elapsed_time");
  g_resources.uniforms.textures[0] =
    glGetUniformLocation(g_resources.program, "textures[0]");
  g_resources.uniforms.textures[1] =
    glGetUniformLocation(g_resources.program, "textures[1]");

  g_resources.attributes.position =
    glGetAttribLocation(g_resources.program, "position");

  return true;
}

void idle() {
  int milliseconds = glutGet(GLUT_ELAPSED_TIME);
  g_resources.elapsed_time = (GLfloat)milliseconds * 0.001f;
  glutPostRedisplay();
}

glm::mat4x4 create_vp() {
  glm::vec3 eyePos(0.0f, 5.f + 5.f * sin(glm::radians(g_resources.elapsed_time * 180)), 10.0f);
  glm::mat4x4 view = glm::lookAt(eyePos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
  view = glm::rotate(view, glm::radians(g_resources.elapsed_time * 60.f), glm::vec3(0.f, 1.f, 0.f));

  float aspect = 4.0f / 3.0f;
  glm::mat4x4 projection = glm::perspective(
      glm::radians(60.0f), 1.0f / aspect, 0.1f, 100.0f);

  return projection * view;
}

void drawPlane(glm::mat4x4 pv, glm::mat4x4 m) {
  glm::mat4x4 mvp = pv * glm::translate(
      glm::mat4x4(), glm::vec3(0.f, 0.f, 0.f)) * m;
  glUniformMatrix4fv(
      g_resources.uniforms.MATRIX_MVP, 1, GL_FALSE, glm::value_ptr(mvp));

  glDrawElements(
      GL_TRIANGLE_STRIP,  /* mode */
      4,                  /* count */
      GL_UNSIGNED_SHORT,  /* type */
      (void*)0            /* element array buffer offset */
      );
}

void render() {
  // Clear the back buffer.
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  // Specify the shader
  glUseProgram(g_resources.program);

  // Set the uniforms
  glUniform1f(g_resources.uniforms.elapsed_time, g_resources.elapsed_time);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, g_resources.textures[0]->GetHandle());
  glUniform1i(g_resources.uniforms.textures[0], 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, g_resources.textures[1]->GetHandle());
  glUniform1i(g_resources.uniforms.textures[1], 1);

  // Set vertex array
  glBindBuffer(GL_ARRAY_BUFFER, g_resources.vertex_buffer);
  glVertexAttribPointer(
      g_resources.attributes.position,  // attribute
      4,                                // size
      GL_FLOAT,                         // type
      GL_FALSE,                         // normalized?
      sizeof(GLfloat)*4,                // stride
      (void*)0                          // array buffer offset
      );
  glEnableVertexAttribArray(g_resources.attributes.position);

  // Set index array
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_resources.element_buffer);

  // Initial transformation matrices.
  glm::mat4x4 xfRotation = glm::rotate(
      glm::mat4x4(),
      glm::radians(g_resources.elapsed_time * 90),
      glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4x4 pv = create_vp();

  // Draw the left model.
  drawPlane(
      pv,
      glm::translate(glm::mat4x4(), glm::vec3(-3.f, 0.f, 0.f)) * xfRotation);

  // Draw the center model.
  drawPlane(
      pv,
      glm::translate(glm::mat4x4(), glm::vec3(0.f, 0.f, 0.f)) * xfRotation);

  // Draw the right model.
  drawPlane(
      pv,
      glm::translate(glm::mat4x4(), glm::vec3(3.f, 0.f, 0.f)) * xfRotation);

  // Clean up
  glDisableVertexAttribArray(g_resources.attributes.position);

  // Present rendered buffer to screen
  glutSwapBuffers();	
}

GLuint make_buffer(
    GLenum target, const void *buffer_data,
    GLsizei buffer_size) {
  GLuint buffer;

  // Generate one new buffer handle.
  glGenBuffers(1, &buffer);

  // Reference this buffer through a particular target.
  glBindBuffer(target, buffer);

  // Allocate the buffer size, with a usage hint indicating
  // that we only write the data once, and only the GPU accesses it.
  glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);

  return buffer;
}

void show_info_log(
    GLuint object, PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog) {
  GLint log_length;

  glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
  char *log = new char[log_length];
  glGet__InfoLog(object, log_length, NULL, log);
  std::cerr << log << std::endl;
  delete[] log;
}

GLuint make_shader(GLenum type, std::string path) {
  std::vector<char> buffer = file_contents(path);
  if (buffer.size() == 0) {
    std::cerr << "Failed to load shader (" << path << ")." << std::endl;
    return 0;
  }
  
  GLuint shader = glCreateShader(type);
  const char* buffer_data = buffer.data();
  GLint length = (GLint)buffer.size();
  glShaderSource(shader, 1, (const GLchar**)&buffer_data, &length);
  glCompileShader(shader);

  GLint shader_ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
  if (!shader_ok) {
    std::cerr << "Failed to compile " << path << ":" << std::endl;
    show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader) {
  GLint program_ok;

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
  if (!program_ok) {
    std::cerr << "Failed to link shader program:" << std::endl;
    show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
    glDeleteProgram(program);
    return 0;
  }
  return program;
}

