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
#define TARGET_FPS 60
#define FRAME_TARGET_TIME 1000/TARGET_FPS
#define FOV 70
#define SPEED_OF_C 1
#define FORCE_MULTIPLIER 42
//#define SPEED_MULTIPLIER 1

typedef enum {
        QUARK_UP,
        QUARK_DOWN,
        QUARK_CHARM,
        QUARK_STRANGE,
        QUARK_TOP,
        QUARK_BOTTOM,
        ELECTRON,
        MUON,
        TAU,
        NEUTRINO_ELECTRON,
        NEUTRINO_MUON,
        NEUTRINO_TAU,
        GLUON,
        PHOTON,
        BOSON_Z,
        BOSON_W,
        HIGGS,
        GRAVITON
}Particle_Type;

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
"   float frequency = 2.0*M_PI*dist*8.0 - 2.0*M_PI*time;\n"
"   alpha *= (sin(frequency + float(identifier)*2.718)-1.0)/13.33 + 1.0;\n"
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


typedef struct Color_RGBA{
        float R;
        float G;
        float B;
        float A;
}Color_RGBA;

typedef struct{
        vec2 position;
        vec2 velocity;
        Particle_Type type;
        int isAntiparticle;
}Particle;

typedef struct{
        Particle* particle;
        unsigned int size;
        unsigned int capacity;
}Particle_Array;

vec3 camera_pos   = {0.0f, 0.0f,  7.0f};
vec3 camera_front = {0.0f, 0.0f, -1.0f};
vec3 camera_up    = {0.0f, 1.0f,  0.0f};
Color_RGBA red_color   = {0.85f, 0.02f, 0.12f, 0.5f};
Color_RGBA color_blue  = {0.12f, 0.12f, 0.85f,1.0f};
Color_RGBA green_color = {0.20f, 1.0f, 0.69f, 0.5f};
Color_RGBA color_purple = {0.9f, 0.7f, 0.98f};
Color_RGBA color_green2 = {0.56f, 0.88f, 0.36f};
Color_RGBA color_orange = {0.96f, 0.52f, 0.4f};
Color_RGBA color_yellow = {0.93f, 0.85f, 0.39f};

void draw_particle(const Particle particle, int ID){
        vec3 position = {particle.position[0], particle.position[1]/1.33, 0.0f};
        float verts[] = {-1.0f, -1.0f, 0.0f,
                          0.0f, 1.0f, 0.0f,
                          1.0f, -1.0f, 0.0f};

        Color_RGBA color;
        switch(particle.type){
                case QUARK_UP:
                case QUARK_DOWN:
                case QUARK_CHARM:
                case QUARK_STRANGE:
                case QUARK_TOP:
                case QUARK_BOTTOM:
                        color = color_purple;
                        break;
                case ELECTRON:
                case MUON:
                case TAU:
                case NEUTRINO_ELECTRON:
                case NEUTRINO_MUON:
                case NEUTRINO_TAU:
                        color = color_green2;
                        break;
                case GLUON:
                case PHOTON:
                case BOSON_Z:
                case BOSON_W:
                case GRAVITON:
                        color = color_orange;
                case HIGGS:
                        color = color_yellow;
                default:
                        break;
        }

        if(particle.isAntiparticle == TRUE){
                color.R = 1.0f - color.R;
                color.G = 1.0f - color.G;
                color.B = 1.0f - color.B;
        }

        mat4 model;
        glm_mat4_identity(model);
        //glm_scale(model, (vec3){0.1f, 0.1f, 1.0f});
        glm_scale(model, (vec3){0.05f, 0.05f, 1.0f});
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
        glUniform1f(time, last_frame_time/1000.0f);
        glUniform1i(part_ID, ID);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*9, verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0 , 3);
}

Particle_Array photons = {NULL, 0};
Particle_Array mesons  = {NULL, 0};
Particle_Array baryons = {NULL, 0};

void init() {
        srand(SDL_GetTicks());
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

void particle_array_push(Particle_Array* array, Particle particle){
        if( array->capacity == 0){
                array->particle = (Particle*)malloc(sizeof(Particle));
                array->particle[0] = particle;
                array->size++;
                array->capacity++;
                return;
        }
        if( array->size == array->capacity ){
                array->particle = (Particle*)realloc(array->particle, sizeof(Particle)*(array->size+1));
                array->capacity++;
        }
        array->particle[array->size] = particle;
        array->size++;
        return;
}

void create_random_particles(Particle_Array* array, const unsigned int quantity){
        for(int i = 0; i < quantity; i++){
                Particle particle;
                particle.position[0] = ((rand() % 98)-49)/50.0f;
                particle.position[1] = ((rand() % 98)-49)/50.0f;
                particle.velocity[0] = ((rand() % 100)-50)/50.0f;
                particle.velocity[1] = ((rand() % 100)-50)/50.0f;
                particle.type = QUARK_UP;
                particle_array_push(array, particle);
        }
}

void spawn_particle(Particle_Array* particles, Particle_Type type, int isAnti, vec2 position, vec2 velocity){
        Particle new_particle;
        new_particle.type = type;
        glm_vec2_copy(position, new_particle.position);
        glm_vec2_copy(velocity, new_particle.velocity);
        new_particle.isAntiparticle = isAnti;
        particle_array_push(particles, new_particle);
}

void spawn_meson(vec2 position1, vec2 velocity1, vec2 position2, vec2 velocity2){
        spawn_particle(&mesons, QUARK_UP, 0, position1, velocity1);
        spawn_particle(&mesons, QUARK_UP, 1, position2, velocity2);
}

void remove_meson(const unsigned int ID){
        if(ID*2 >= mesons.size){
                printf("ERROR: Meson ID greater than size\n");
                exit(1);
        }
        if(ID == (mesons.size/2) - 1){
                mesons.size -= 2;
                return;
        }
        mesons.particle[ID*2]     = mesons.particle[mesons.size-2];
        mesons.particle[(ID*2)+1] = mesons.particle[mesons.size-1];
        mesons.size -= 2;
        return;
}

void spawn_baryon(){
        for(int i = 0; i < 3; i++){
                float positionX = ((rand() % 98)-49)/50.0f;
                float positionY = ((rand() % 98)-49)/50.0f;
                float velX = ((rand() % 98)-49)/50.0f;
                float velY = ((rand() % 98)-49)/50.0f;
                spawn_particle(&baryons, QUARK_UP, FALSE, (vec2){positionX, positionY}, (vec2){velX,velY});
        }
}

void spawn_photon(vec2 position, vec2 velocity){
        spawn_particle(&photons, PHOTON, FALSE, position, velocity);
}
// When destroying copy the last place to here and pop it

void check_boundaries(Particle_Array particles){
        for(int i = 0; i < particles.size; i++){
                vec2 velocity;
                glm_vec2_copy(particles.particle[i].velocity, velocity);
                if(particles.particle[i].position[0] > 1 && velocity[0] > 0) particles.particle[i].velocity[0] *= -1;
                if(particles.particle[i].position[0] < -1 && velocity[0] < 0) particles.particle[i].velocity[0] *= -1;
                if(particles.particle[i].position[1] > 1 && velocity[1] > 0) particles.particle[i].velocity[1] *= -1;
                if(particles.particle[i].position[1] < -1 && velocity[1] < 0) particles.particle[i].velocity[1] *= -1;
        }
}

void strong_force_produce_pair(){
}

void update_photons(float delta_time){
        for(int i = 0; i < photons.size; i++){
                Particle part1 = photons.particle[i];
                glm_vec2_normalize(part1.velocity);
                glm_vec2_scale(part1.velocity, SPEED_OF_C, part1.velocity);
                photons.particle[i] = part1;
                photons.particle[i].position[0] += photons.particle[i].velocity[0]*delta_time;
                photons.particle[i].position[1] += photons.particle[i].velocity[1]*delta_time;
        }
        check_boundaries(photons);
}

void update_baryons(float delta_time){
        if(baryons.size % 3 != 0) exit(1); // Baryons need 3 quarks

        // Strong force between 3 quarks
        for(int i = 0; i < baryons.size/3; i++){
                vec2 force[] = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};

                Particle part[3];
                part[0] = baryons.particle[i*3];
                part[1] = baryons.particle[i*3 + 1];
                part[2] = baryons.particle[i*3 + 2];

                float mid_x = (part[0].position[0] + part[1].position[0] + part[2].position[0])/3;
                float mid_y = (part[0].position[1] + part[1].position[1] + part[2].position[1])/3;
                vec2 middle_point = {mid_x, mid_y};

                for(int j = 0; j < 3; j++){
                        float dist_squared = glm_vec2_distance2(part[j].position, middle_point);
                        float max_dist = 0.2;
                        // Pair creation
                        if(dist_squared > pow(max_dist,2)){
                                vec2 middle_middle;
                                glm_vec2_add(middle_point, part[j].position, middle_middle);
                                glm_vec2_scale(middle_middle, 0.5, middle_middle);
                                spawn_meson(part[j].position, part[j].velocity, middle_middle, part[j].velocity);
                                glm_vec2_copy(middle_middle, part[j].position);
                                // Update velocity too 
                        }
                }

                // Attraction forces between quarks 
                for(int j = 0; j < 3; j++){
                        for(int k = 0; k < 2; k++){
                                float dist_squared = glm_vec2_distance2(part[j].position, part[(j+1+k)%3].position);
                                float min_dist = 0.055;
                                if(dist_squared <= pow(min_dist,2)) continue;  // Asymptotic freedom
                                vec2 forceDir[2];
                                glm_vec2_sub(part[j].position, part[(j+1+k)%3].position, forceDir[k]);
                                glm_vec2_norm(forceDir[k]);

                                float forceMag = -FORCE_MULTIPLIER;
                                glm_vec2_scale(forceDir[k], forceMag, forceDir[k]);
                                glm_vec2_add(force[j], forceDir[k], force[j]);
                        }
                }

                for(int j = 0; j < 3; j++){
                        // Drag
                        vec2 drag = {0.0f, 0.0f};
                        glm_vec2_copy(part[j].velocity, drag);
                        glm_vec2_negate(drag);
                        glm_vec2_scale(drag, 0.1, drag);
                        glm_vec2_add(force[j], drag, force[j]);

                        // UPDATE VELOCITIES
                        glm_vec2_scale(force[j], delta_time, force[j]);
                        glm_vec2_add(part[j].velocity, force[j], part[j].velocity); 

                        // Max Velocity
                        float mag_squared = glm_vec2_norm2(part[j].velocity);
                        if(mag_squared > pow(SPEED_OF_C,2)){
                                glm_vec2_normalize(part[j].velocity);
                                glm_vec2_scale(part[j].velocity, SPEED_OF_C, part[j].velocity);
                        }
                }
                baryons.particle[i*3]     = part[0];
                baryons.particle[i*3 + 1] = part[1];
                baryons.particle[i*3 + 2] = part[2];
        }

        // UPDATE POSITIONS
        for(int i = 0; i < baryons.size; i++){
                Particle part1 = baryons.particle[i];
                baryons.particle[i].position[0] += baryons.particle[i].velocity[0]*delta_time;
                baryons.particle[i].position[1] += baryons.particle[i].velocity[1]*delta_time;
        }
        // BOUNDARIES
        check_boundaries(baryons);
}

void update_mesons(float delta_time){
        if(mesons.size % 2 != 0) exit(1); // Mesons need 2 quarks

        vec2 force[] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
        for(int i = 0; i < mesons.size/2; i++){
                Particle part[2];
                part[0] = mesons.particle[i*2];
                part[1] = mesons.particle[i*2 + 1];

                // Meson annihilation
                float dist_squared = glm_vec2_distance2(part[0].position, part[1].position);
                float min_dist = 0.05;
                if(dist_squared <= pow(min_dist,2)){
                        vec2 new_vel = {part[0].velocity[1], -part[0].velocity[0]};
                        vec2 new_vel2 = {-part[0].velocity[1], part[0].velocity[0]};
                        spawn_photon(part[0].position, new_vel);
                        spawn_photon(part[0].position, new_vel2);
                        remove_meson(i);
                        continue;
                }

                vec2 forceDir;
                glm_vec2_sub(part[0].position, part[1].position, forceDir);
                glm_vec2_norm(forceDir);

                float forceMag = -FORCE_MULTIPLIER;

                glm_vec2_scale(forceDir, forceMag, forceDir);
                glm_vec2_add(force[0], forceDir, force[0]);
                glm_vec2_sub(force[1], forceDir, force[1]);

                for(int j = 0; j < 2; j++){
                        // Drag
                        vec2 drag = {0.0f, 0.0f};
                        glm_vec2_copy(part[j].velocity, drag);
                        glm_vec2_negate(drag);
                        glm_vec2_scale(drag, 0.1, drag);
                        glm_vec2_add(force[j], drag, force[j]);

                        // UPDATE VELOCITIES
                        glm_vec2_scale(force[j], delta_time, force[j]);
                        glm_vec2_add(part[j].velocity, force[j], part[j].velocity); 

                        // Max Velocity
                        float mag_squared = glm_vec2_norm2(part[j].velocity);
                        if(mag_squared > pow(SPEED_OF_C,2)){
                                glm_vec2_normalize(part[j].velocity);
                                glm_vec2_scale(part[j].velocity, SPEED_OF_C, part[j].velocity);
                        }
                }
                mesons.particle[i*2]     = part[0];
                mesons.particle[i*2 + 1] = part[1];
        }

        // UPDATE POSITIONS
        for(int i = 0; i < mesons.size; i++){
                Particle part1 = mesons.particle[i];
                mesons.particle[i].position[0] += mesons.particle[i].velocity[0]*delta_time;
                mesons.particle[i].position[1] += mesons.particle[i].velocity[1]*delta_time;
        }
        // BOUNDARIES
        check_boundaries(mesons);
}

void update(){

        int wait_time = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);
        if(wait_time > 0 && wait_time <= FRAME_TARGET_TIME)
                SDL_Delay(wait_time);

        float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
        last_frame_time = SDL_GetTicks();

        float currentTime = SDL_GetTicks();
        if(currentTime - lastTime >= 1*1000.0f){
                lastTime = currentTime;
                float v1 = ((rand() % 98)-49)/50.0f;
                float v2 = ((rand() % 98)-49)/50.0f;
                float p1 = ((rand() % 98)-49)/50.0f;
                float p2 = ((rand() % 98)-49)/50.0f;
                //spawn_meson((vec2){p1,p2}, (vec2){v1,v2}, (vec2){v1,v2}, (vec2){p1,p2});
                //spawn_baryon();
                //spawn_particle(&photons, PHOTON, 0, (vec2){0.0f,0.0f}, (vec2){10.0f,10.0f});
        }

        if(delta_time > 0.05) return;
        delta_time *= 1;
        update_photons(delta_time);
        update_baryons(delta_time);
        update_mesons(delta_time);

}

typedef struct{
        char* string;
        unsigned int size;
} String;


void draw(){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthMask(GL_FALSE); // Disable for particles because it shows their triangles
        for(int i = 0; i < baryons.size; i++){
                draw_particle(baryons.particle[i], i);
        }
        for(int i = 0; i < photons.size; i++){
                draw_particle(photons.particle[i], i);
        }
        for(int i = 0; i < mesons.size; i++){
                draw_particle(mesons.particle[i], i);
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

        for(int i = 0; i < 10; i++){
                spawn_baryon();
        }

        while(quit == FALSE){
                input(&quit);

                update();
                //nk_end(ctx);

                draw();
                //nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        }
        //nk_sdl_shutdown();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(glWindow);
        SDL_Quit();
        return 0;
}
