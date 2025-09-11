#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <glad/glad.h>
#include <unistd.h>

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

SDL_Window*   glWindow = NULL;
SDL_GLContext glContext = NULL;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "   gl_PointSize = 16.0;\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";

float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};
unsigned int vertexShader;
unsigned int fragmentShader;
unsigned int shaderProgram;
unsigned int VAO;
unsigned int VBO;

void init(void);
void reshape(void);
void display(void);

void drawSegmentByLineEquation(float x1, float y1, float x2, float y2){
  float dx = (x2 - x1);
  float dy = (y2 - y1);
  float m = dy / dx;
  float y = y1;

  int resolution = 200;

  float verts[(resolution*3)+3];

  float step = dx / resolution;
  int i = 0;
  for (float x = x1; x <= x2; x += step) {
    y += m / resolution;
    verts[i*3] = x;
    verts[i*3 + 1] = y;
    verts[i*3 + 2] = 0.0f;
    i++;
  }
  
  glUseProgram(shaderProgram);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_POINTS, 0 , resolution);
}


void init() {

  int success;
  char infoLog[512];
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glDeleteShader(vertexShader);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success){
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);
  }
  
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success){
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);


  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
}

int main(int argc, char** argv) {
  if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
    printf("SDL2 could not initialize video subsystem\n");
    exit(1);
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  glWindow = SDL_CreateWindow("Window", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
  if(glWindow == NULL){
    printf("Error creating window\n");
    exit(1);
  }
  glContext = SDL_GL_CreateContext(glWindow);
  if(glContext == NULL){
    printf("Error creating GL context\n");
    exit(1);
  }

  if(!gladLoadGLLoader(SDL_GL_GetProcAddress)){
        puts("glad was not initialized");
        exit(1);
  }
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);


  init();

  int quit = 0;
  while(quit == 0){
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0){
      if(e.type == SDL_QUIT){
        quit = 1;
      }
    }
    
    drawSegmentByLineEquation(-0.5f, -0.1f, 0.5f, 0.1f);
    SDL_GL_SwapWindow(glWindow);
  }
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(glWindow);
  SDL_Quit();
  return 0;
}
