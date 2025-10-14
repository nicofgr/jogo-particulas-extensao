#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <glad/glad.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // for wait time
#include <cglm/cglm.h>

#include "cglm/cam.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"

// Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024
//#include "nuklear/nuklear.h"
//#include "nuklear/nuklear_sdl_gl3.h"

// My defines
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define TRUE  1
#define FALSE 0
#define TARGET_FPS 90
#define FRAME_TARGET_TIME 1000/TARGET_FPS
#define FOV 70

int rotate = TRUE;
float rotate_speed = -1.0f/30; // frequency
vec3 translation = {0.0f, 0.0f, 0.0f};

SDL_Window*   glWindow = NULL;
SDL_GLContext glContext = NULL;

float yaw = -90.0f;
float pitch = 0.0f;

const char *vertexShaderSource = "#version 100\n"
"attribute vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float sizeMultiplier;\n"
"varying vec3 pos;\n"
"void main()\n"
"{\n"
"   gl_Position = projection*view*model*vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   gl_PointSize = 3.0;\n"
"   pos = aPos;\n"
"}\0";

const char *fragmentShaderSource = "#version 100\n"
"precision mediump float;\n"
"varying vec3 pos;\n"
"uniform vec4 ourColor;\n"
"uniform float time;\n"
"uniform int identifier;\n"
"void main()\n"
"{\n"
"   const float M_PI = 3.141592;\n"
"   float radius = 0.35;\n"
"   float dist = length(pos);\n"
"   float alpha = 1.0 - smoothstep(radius-0.3, radius, dist);\n"
"   float frequency = 2.0*M_PI*dist;\n"
"   alpha *= (sin(frequency*8.0 - time*20.0 + float(identifier)*2.718)-1.0)/13.33 + 1.0;\n"
"   gl_FragColor = vec4(ourColor.xyz*alpha, alpha);\n"
"}\0";

unsigned int vertexShader;
unsigned int fragmentShader;
unsigned int shaderProgram;
unsigned int VAO;
unsigned int VBO;

unsigned int VAO_faces;
unsigned int VBO_faces;
unsigned int EBO_faces;

int last_frame_time = 0;
int lastTime = 0;
struct nk_context *ctx;

int option_selected = 0;
int step_draw = TRUE;
int selected_vertex = 0;
int selected_face = 0;
int selected_edge = 0;

typedef struct Color_RGBA{
        float R;
        float G;
        float B;
        float A;
}Color_RGBA;

vec3 camera_pos   = {0.0f, 0.0f,  7.0f};
vec3 camera_front = {0.0f, 0.0f, -1.0f};
vec3 camera_up    = {0.0f, 1.0f,  0.0f};
Color_RGBA red_color   = {0.85f, 0.02f, 0.12f, 0.5f};
Color_RGBA green_color = {0.20f, 1.0f, 0.69f, 0.5f};


typedef struct{
        vec2 position;
        vec2 velocity;
        Color_RGBA color;
}Particle;

void draw_particle(const Particle particle, int ID){
        vec3 position = {particle.position[0], particle.position[1]/1.33, 0.0f};
        float verts[] = {-1.0f, -1.0f, 0.0f,
                          0.0f, 1.0f, 0.0f,
                          1.0f, -1.0f, 0.0f};
        Color_RGBA color = particle.color;
        const float point_size_multiplier = 1;

        mat4 model;
        glm_mat4_identity(model);
        glm_scale(model, (vec3){0.1f, 0.1f, 1.0f});
        //glm_scale(model, (vec3){0.2f, 0.2f, 1.0f});

        mat4 world;
        glm_mat4_identity(world);
        glm_translate(world, (float*)position);
        glm_mat4_mul(world, model, model);

        mat4 view;   // Camera space
        glm_mat4_identity(view);

        // camera pos global
        vec3 target_dir;
        vec3 direction = {0.0f, 0.0f, -1.0f}; // mouse dir
        glm_normalize_to(direction, camera_front); // mouse dir is normalized to camera front
        glm_vec3_add(camera_pos, camera_front, target_dir);
        glm_lookat(camera_pos, target_dir, camera_up, view);

        /////////////////////////////////////////////////////////////////////
        mat4 proj;  // Clip space
        glm_mat4_identity(proj);
        glm_ortho(-1,1,-1.0/1.33,1.0/1.33, 0, 100, proj);

        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        int transformLocation   = glGetUniformLocation(shaderProgram, "model");
        int viewLocation        = glGetUniformLocation(shaderProgram, "view");
        int projLocation        = glGetUniformLocation(shaderProgram, "projection");
        int sizeMultiplier      = glGetUniformLocation(shaderProgram, "sizeMultiplier");
        int time                = glGetUniformLocation(shaderProgram, "time");
        int part_ID             = glGetUniformLocation(shaderProgram, "identifier");
        glUseProgram(shaderProgram);
        glUniform4f(vertexColorLocation, color.R, color.G, color.B, color.A);
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, (const float*)model);
        glUniformMatrix4fv(viewLocation     , 1, GL_FALSE, (const float*)view);
        glUniformMatrix4fv(projLocation     , 1, GL_FALSE, (const float*)proj);
        glUniform1f(sizeMultiplier, point_size_multiplier);
        glUniform1f(time, last_frame_time/1000.0f);
        glUniform1i(part_ID, ID);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*9, verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0 , 3);
}

void init() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        //glEnable(GL_PROGRAM_POINT_SIZE);
        //glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        int success;
        char infoLog[512];
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if(!success){
                glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
                printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s\n", infoLog);
                printf("%d\n", glGetError());
                exit(1);
        }

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if(!success){
                glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
                printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s\n", infoLog);
                exit(1);
        }

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if(!success) {
                glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n %s\n", infoLog);
                exit(1);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);


        //glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        //glGenVertexArrays(1, &VAO_faces);
        glGenBuffers(1, &VBO_faces);
        glGenBuffers(1, &EBO_faces);


        //glBindVertexArray(VAO);
        /**
          glBindBuffer(GL_ARRAY_BUFFER, VBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
         **/

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

}
void input(int * quit){
        SDL_Event e;
        //nk_input_begin(ctx);
        const float camera_speed = 0.1f;
        const Uint8* states = SDL_GetKeyboardState(NULL);
        while(SDL_PollEvent(&e)){
                switch(e.type){
                        case SDL_QUIT:
                                *quit = TRUE;
                                break;
                        case SDL_KEYDOWN:
                                if(e.key.keysym.sym == SDLK_ESCAPE)
                                        *quit = TRUE;
                                if(e.key.keysym.sym == SDLK_TAB){
                                        if(SDL_GetRelativeMouseMode() == SDL_TRUE){
                                                SDL_SetRelativeMouseMode(SDL_FALSE);
                                        }else{
                                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                                SDL_GetRelativeMouseState(NULL, NULL);
                                        }
                                }
                                break;  
                }
                //nk_sdl_handle_event(&e);
        }

        const float sensitivity = 0.25;
        int x = 0, y = 0;
        if(SDL_GetRelativeMouseMode() == SDL_TRUE)
                SDL_GetRelativeMouseState(&x, &y);
        yaw += x*sensitivity;
        pitch -= y*sensitivity;
        if(pitch > 89.9f)
                pitch = 89.9f;
        if(pitch < -89.9f)
                pitch = -89.9f;
        if(0){
        printf("%d, %d\n", x, y);
        printf("Pos: %.2f, %.2f, %.2f\n", camera_pos[0], camera_pos[1], camera_pos[2]);
        printf("Fnt: %.2f, %.2f, %.2f\n", camera_front[0], camera_front[1], camera_front[2]);
        printf("Up:  %.2f, %.2f, %.2f\n\n", camera_up[0], camera_up[1], camera_up[2]);
        }
        //nk_sdl_handle_grab();
        //nk_input_end(ctx);
}

typedef struct{
        Particle* particle;
        unsigned int size;
}Particle_Array;

void particle_array_push(Particle_Array* array, Particle particle){
        if( array->size == 0){
                array->particle = (Particle*)malloc(sizeof(Particle));
                array->particle[0] = particle;
                array->size++;
                return;
        }
        array->particle = (Particle*)realloc(array->particle, sizeof(Particle)*(array->size+1));
        array->particle[array->size] = particle;
        array->size++;
        return;
}

void create_random_particles(Particle_Array* array, const unsigned int quantity){
        for(int i = 0; i < quantity; i++){
                Particle particle;
                particle.position[0] = ((rand() % 98)-49)/55.0f;
                particle.position[1] = ((rand() % 98)-49)/55.0f;
                particle.color = red_color;
                particle.velocity[0] = 0.0f;
                particle.velocity[1] = 0.0f;
                particle_array_push(array, particle);
        }
}

void update( Particle_Array particles){

        int wait_time = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);
        if(wait_time > 0 && wait_time <= FRAME_TARGET_TIME)
                SDL_Delay(wait_time);

        float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
        last_frame_time = SDL_GetTicks();

        for(int i = 0; i < particles.size; i++){
                Particle part1 = particles.particle[i];
                vec2 force = {0.0f, 0.0f};

                // UPDATE FORCES
                for(int j = 0; j < particles.size; j++){
                        if(i == j) continue;
                        Particle part2 = particles.particle[j];
                        vec2 forceDir;
                        glm_vec2_sub(part1.position, part2.position, forceDir);
                        glm_vec2_norm(forceDir);

                        float dist_squared = glm_vec2_distance2(part1.position, part2.position);
                        float maxForce = 1.0; // <<
                        float minDist = 0.1;
                        float forceMag = - (maxForce*minDist*minDist)/dist_squared;
                        if(dist_squared < minDist*minDist){
                                forceMag = -forceMag + 2*maxForce ;
                        }

                        glm_vec2_scale(forceDir, forceMag, forceDir);
                        glm_vec2_add(force, forceDir, force);

                }
                vec2 drag = {0.0f, 0.0f};
                glm_vec2_copy(part1.velocity, drag);
                glm_vec2_negate(drag);
                glm_vec2_scale(drag, 0.5, drag);
                glm_vec2_add(force, drag, force);

                // UPDATE VELOCITIES
                glm_vec2_scale(force, delta_time, force);
                glm_vec2_add(part1.velocity, force, part1.velocity); 
                particles.particle[i] = part1;

                // UPDATE POSITIONS
                if(delta_time > 1.0) return;
                particles.particle[i].position[0] += particles.particle[i].velocity[0]*delta_time;
                particles.particle[i].position[1] += particles.particle[i].velocity[1]*delta_time;

                // BOUNDARIES
                vec2 velocity;
                glm_vec2_copy(particles.particle[i].velocity, velocity);
                if(particles.particle[i].position[0] > 1 && velocity[0] > 0) particles.particle[i].velocity[0] *= -1;
                if(particles.particle[i].position[0] < -1 && velocity[0] < 0) particles.particle[i].velocity[0] *= -1;
                if(particles.particle[i].position[1] > 1 && velocity[1] > 0) particles.particle[i].velocity[1] *= -1;
                if(particles.particle[i].position[1] < -1 && velocity[1] < 0) particles.particle[i].velocity[1] *= -1;
        }
}

typedef struct{
        char* string;
        unsigned int size;
} String;


void draw(const Particle_Array particles){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthMask(GL_FALSE); // Disable for particles because it shows their triangles
                for(int i = 0; i < particles.size; i++){
                draw_particle(particles.particle[i], i);
        }
        glDepthMask(GL_TRUE);
        SDL_GL_SwapWindow(glWindow);
}

int main(int argc, char** argv) {
        if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
                printf("SDL2 could not initialize video subsystem\n");
                exit(1);
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
        glWindow = SDL_CreateWindow("OpenGL ES 2.0", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
        if(glWindow == NULL){
                printf("Error creating window\n");
                exit(1);
        }

        glContext = SDL_GL_CreateContext(glWindow);
        if(glContext == NULL){
                printf("Error creating GL context\n");
                exit(1);
        }

        if(!gladLoadGLES2Loader(SDL_GL_GetProcAddress)){
                puts("glad was not initialized");
                exit(1);
        }
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);


        int quit = FALSE;
        int counter = 0;

        init();

        /**
        ctx = nk_sdl_init(glWindow);
        {struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();}
        **/

        Particle_Array particles = {NULL, 0};
        create_random_particles(&particles,10);
        while(quit == FALSE){
                input(&quit);

                update(particles);
                //nk_end(ctx);

                draw(particles);
                //nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        }
        //nk_sdl_shutdown();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(glWindow);
        SDL_Quit();
        return 0;
}
