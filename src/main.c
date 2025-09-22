//
//#include "cglm/affine-pre.h"
//#include "cglm/affine-pre.h"
#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
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

#include "half_edge.h"

#include "obj_loader.h"

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
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"

// My defines
#define SCREEN_WIDTH   600
#define SCREEN_HEIGHT  600
#define TRUE  1
#define FALSE 0
#define TARGET_FPS 90
#define FRAME_TARGET_TIME 1000/TARGET_FPS
#define FOV 70

int rotate = FALSE;
float rotate_speed = -1.0f/0.9; // frequency
vec3 translation = {0.0f, 0.0f, 0.0f};

SDL_Window*   glWindow = NULL;
SDL_GLContext glContext = NULL;

float yaw = -90.0f;
float pitch = 0.0f;

const char *vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float sizeMultiplier;\n"
"void main()\n"
"{\n"
"   gl_Position = projection*view*model*vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   gl_PointSize = min((50*sizeMultiplier)/gl_Position.z, 400.0f);\n"
"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = ourColor;\n"
"}\0";

unsigned int vertexShader;
unsigned int fragmentShader;
unsigned int shaderProgram;
unsigned int VAO;
unsigned int VBO;

unsigned int VAO_faces;
unsigned int VBO_faces;
unsigned int EBO_faces;

HE_Object current_object;
char* opened_file = NULL;
int last_frame_time = 0;
int lastTime = 0;
struct nk_context *ctx;


void init(void);
void reshape(void);
void display(void);

void swapFloat(float* x1 , float* x2){
        float aux = *x1;
        *x1 = *x2;
        *x2 = aux;
}

typedef struct Color_RGBA{
        float R;
        float G;
        float B;
        float A;
}Color_RGBA;

vec3 camera_pos   = {0.0f, 0.0f,  7.0f};
vec3 camera_front = {0.0f, 0.0f, -1.0f};
vec3 camera_up    = {0.0f, 1.0f,  0.0f};
Color_RGBA red_color = {0.85f, 0.02f, 0.12f, 0.5f};
Color_RGBA green_color = {0.20f, 1.0f, 0.69f, 0.5f};

void drawSegmentByLineEquation(float x1, float y1, float z1, float x2, float y2, float z2, const unsigned int resolution, const float point_size_multiplier, const Color_RGBA color){
        float a = x2-x1;
        float b = y2-y1;
        float c = z2-z1;

        float x = x1;
        float y = y1;
        float z = z1;

        float verts[(resolution*3)+3];
        float step = 1.0f / (resolution-1);
        for (int i = 0; i <= resolution-1; i++) { // From 0 till res-1
                verts[i*3 + 0] = x;
                verts[i*3 + 1] = y;
                verts[i*3 + 2] = z;
                x += step*a;
                y += step*b;
                z += step*c;
        }

        mat4 model;
        glm_mat4_identity(model);
        //glm_scale(model, (vec4){0.1f, 0.1f, 0.1f, 1.0f});
        glm_translate(model, translation);
        if(rotate == TRUE)
                glm_rotate(model, rotate_speed*((float)SDL_GetTicks()/1000.0f)*GLM_PI*2, (vec3){0.0f, 1.0f, 0.0f});

        mat4 view;
        glm_mat4_identity(view);
        //glm_translate(view, translation);

        vec3 target_dir;

        vec3 direction;
        direction[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
        direction[1] = sin(glm_rad(pitch));
        direction[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
        glm_normalize_to(direction, camera_front);
        glm_vec3_add(camera_pos, camera_front, target_dir);
        glm_lookat(camera_pos, target_dir, camera_up, view);

        mat4 proj;
        glm_mat4_identity(proj);
        glm_perspective(glm_rad(FOV), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f, proj);

        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        int transformLocation   = glGetUniformLocation(shaderProgram, "model");
        int viewLocation        = glGetUniformLocation(shaderProgram, "view");
        int projLocation        = glGetUniformLocation(shaderProgram, "projection");
        int sizeMultiplier        = glGetUniformLocation(shaderProgram, "sizeMultiplier");
        glUseProgram(shaderProgram);
        glUniform4f(vertexColorLocation, color.R, color.G, color.B, color.A);
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, (const float*)model);
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, (const float*)view);
        glUniformMatrix4fv(projLocation, 1, GL_FALSE, (const float*)proj);
        glUniform1f(sizeMultiplier, point_size_multiplier);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_POINTS, 0 , resolution);
}

float* HE_get_object_verts_as_array(HE_Object object){
        HE_Vertex_Array verts = object.vertex_array;
        float* output = (float*)malloc(sizeof(float)*3*verts.size);
        for(int i = 0; i < verts.size; i++){
                output[i*3]       = verts.array[i].x;
                output[(i*3) + 1] = verts.array[i].y;
                output[(i*3) + 2] = verts.array[i].z;
        }
        return output;
}


void draw_face(const Color_RGBA color, const unsigned int face){
        float* verts = HE_get_object_verts_as_array(current_object);
        int size = current_object.vertex_array.size*3;

        int face_data_size;
        //unsigned int* face_data = HE_get_object_faces_as_array(current_object, &face_data_size);
        unsigned int* face_data = HE_get_object_single_face_as_array(current_object, &face_data_size, face);

        mat4 model;
        glm_mat4_identity(model);
        //glm_scale(model, (vec4){0.1f, 0.1f, 0.1f, 1.0f});
        glm_translate(model, translation);
        if(rotate == TRUE)
                glm_rotate(model, rotate_speed*((float)SDL_GetTicks()/1000.0f)*GLM_PI*2, (vec3){0.0f, 1.0f, 0.0f});

        mat4 view;
        glm_mat4_identity(view);
        glm_translate(view, translation);

        vec3 target_dir;

        vec3 direction;
        direction[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
        direction[1] = sin(glm_rad(pitch));
        direction[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
        glm_normalize_to(direction, camera_front);
        glm_vec3_add(camera_pos, camera_front, target_dir);
        glm_lookat(camera_pos, target_dir, camera_up, view);

        mat4 proj;
        glm_mat4_identity(proj);
        glm_perspective(glm_rad(FOV), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f, proj);

        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        int transformLocation   = glGetUniformLocation(shaderProgram, "model");
        int viewLocation        = glGetUniformLocation(shaderProgram, "view");
        int projLocation        = glGetUniformLocation(shaderProgram, "projection");
        int sizeMultiplier        = glGetUniformLocation(shaderProgram, "sizeMultiplier");
        glUseProgram(shaderProgram);
        glUniform4f(vertexColorLocation, color.R, color.G, color.B, color.A);
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, (const float*)model);
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, (const float*)view);
        glUniformMatrix4fv(projLocation, 1, GL_FALSE, (const float*)proj);
        glUniform1f(sizeMultiplier, 1);

        glBindVertexArray(VAO_faces);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_faces);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*size, verts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_faces);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*face_data_size, face_data, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glDrawElements(GL_TRIANGLES, face_data_size , GL_UNSIGNED_INT, 0);
        free(verts);
        free(face_data);
}

void draw_adjacent_face_from_face(const int face){
        draw_face(red_color, face);
        HE_Edge_Array edge_array = current_object.edge_array;
        int starting_edge = current_object.face_array.array[face].edge_ID;
        int next_edge = edge_array.array[starting_edge].nextEdge_ID;
        int next_edge_twin = edge_array.array[next_edge].twin_edge_ID;
        int next_edge_twin_face = edge_array.array[next_edge_twin].inc_face_ID;
        if(next_edge_twin_face != -1)
                draw_face(green_color, next_edge_twin_face);
        while(starting_edge != next_edge){
                next_edge = edge_array.array[next_edge].nextEdge_ID;
                next_edge_twin = edge_array.array[next_edge].twin_edge_ID;
                next_edge_twin_face = edge_array.array[next_edge_twin].inc_face_ID;
                if(next_edge_twin_face != -1)
                        draw_face(green_color, next_edge_twin_face);
        }
}

void draw_adjacent_face_from_edge(const int edge){
        HE_Edge_Array edge_array = current_object.edge_array;
        int twin_edge = edge_array.array[edge].twin_edge_ID;
        int my_face = edge_array.array[edge].inc_face_ID;
        int twin_face = edge_array.array[twin_edge].inc_face_ID;
        draw_face(green_color, my_face);
        if(twin_face != -1)
                draw_face(green_color, twin_face);
}
void draw_adjacent_face_from_vertex(const int vertex){
        HE_Edge_Array edge_array = current_object.edge_array;
        int starting_edge      = current_object.vertex_array.array[vertex].inc_edge_ID;
        int previous_edge      = edge_array.array[starting_edge].prvsEdge_ID;
        int twin_previous_edge = edge_array.array[previous_edge].twin_edge_ID;
        int face = edge_array.array[twin_previous_edge].inc_face_ID;
        if(face != -1)
                draw_face(green_color, face);
        while(starting_edge != twin_previous_edge){
                previous_edge      = edge_array.array[twin_previous_edge].prvsEdge_ID;
                twin_previous_edge = edge_array.array[previous_edge].twin_edge_ID;
                face = edge_array.array[twin_previous_edge].inc_face_ID;
                if(face != -1)
                        draw_face(green_color, face);

        }
}

void update_itemlist(char*** orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.vertex_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "v", i+1);
                //printf("%s ", str_buff);
                items[i] = str_buff;
        }
        *orig = items;
}

void update_facelist(char***orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.face_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "f", i);
                //printf("%s ", str_buff);
                items[i] = str_buff;
        }
        *orig = items;
}

void update_edgelist(char***orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.edge_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "e", i);
                items[i] = str_buff;
        }
        *orig = items;
}


void init() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_MULTISAMPLE);
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


        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glGenVertexArrays(1, &VAO_faces);
        glGenBuffers(1, &VBO_faces);
        glGenBuffers(1, &EBO_faces);


        glBindVertexArray(VAO);
        /**
          glBindBuffer(GL_ARRAY_BUFFER, VBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
         **/

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        current_object = HE_load("test.obj");
}
int update_item = FALSE;

void input(int * quit){
        SDL_Event e;
        nk_input_begin(ctx);
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
                        case SDL_DROPFILE:
                                if(opened_file != NULL)
                                        free(opened_file);
                                opened_file = (char*)malloc(sizeof(char)*(strlen(e.drop.file)+1));
                                strcpy(opened_file, e.drop.file);
                                printf("File dropped: %s\n", opened_file);
                                free(current_object.vertex_array.array);
                                free(current_object.face_array.array);
                                free(current_object.edge_array.array);
                                current_object = HE_load(opened_file);
                                update_item = TRUE;
                                SDL_free(e.drop.file);
                                break;
                }
                nk_sdl_handle_event(&e);
        }
        if(states[SDL_SCANCODE_W]){
                vec3 test;
                glm_vec3_copy(camera_front, test);
                test[1] = 0.0f;
                glm_vec3_muladds(test, camera_speed, camera_pos); //pos += (front*spd)
        }
        if(states[SDL_SCANCODE_S]){
                vec3 test;
                glm_vec3_copy(camera_front, test);
                test[1] = 0.0f;
                glm_vec3_muladds(test, -camera_speed, camera_pos);
        }
        if(states[SDL_SCANCODE_A]){
                vec3 aux;
                glm_vec3_crossn(camera_front, camera_up, aux);
                glm_vec3_muladds(aux, -camera_speed, camera_pos);
        }
        if(states[SDL_SCANCODE_D]){
                vec3 aux;
                glm_vec3_crossn(camera_front, camera_up, aux);
                glm_vec3_muladds(aux, camera_speed, camera_pos);
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
        nk_sdl_handle_grab();
        nk_input_end(ctx);
}

void update(int* step){

        int wait_time = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);
        if(wait_time > 0 && wait_time <= FRAME_TARGET_TIME)
                SDL_Delay(wait_time);

        float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
        last_frame_time = SDL_GetTicks();

        *step = 0;
        float currentTime = SDL_GetTicks();
        if(currentTime - lastTime >= 500.0f){ // Updates every x seconds
                *step = 1;
                lastTime = SDL_GetTicks();
        }
}

typedef struct{
        char* string;
        unsigned int size;
} String;

int option_selected = 0;
int step_draw = TRUE;
int selected_vertex = 0;
int selected_face = 0;
int selected_edge = 0;
void HE_draw(const HE_Object object){
        HE_Edge_Array   edgeArray = object.edge_array;
        HE_Vertex_Array verArray  = object.vertex_array;
        HE_Face_Array   faceArray = object.face_array;

        float scds = 8;  // Time to draw whole figure
        float cnt = (int)(SDL_GetTicks()/((scds*1000.0f)/edgeArray.size))%(edgeArray.size) + 1;
        int step = 0;

        if(step_draw == FALSE)
                cnt = edgeArray.size;
        for(int e = 0; e <= edgeArray.size; e++){
                if(step == cnt)
                        break;
                int originVertex = edgeArray.array[e].origin_vertex_ID;
                float x1 = verArray.array[originVertex].x;
                float y1 = verArray.array[originVertex].y;
                float z1 = verArray.array[originVertex].z;

                int nextEdge = edgeArray.array[e].nextEdge_ID;
                int nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = verArray.array[nextVertex].x;
                float y2 = verArray.array[nextVertex].y;
                float z2 = verArray.array[nextVertex].z;
                //printf("From v%d to v%d\n", originVertex+1, nextVertex+1);
                //printf("v%d: %.2f %.2f %.2f\n", originVertex+1, x1, y1, z1);
                //printf("v%d: %.2f %.2f %.2f\n", nextVertex+1, x2, y2, z2);
                if (step_draw == FALSE && (originVertex == selected_vertex || nextVertex == selected_vertex) && option_selected == 3){
                        drawSegmentByLineEquation(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.02f, 0.12f, 1.0f});
                }else if (step == cnt-1 && step_draw == TRUE){
                        drawSegmentByLineEquation(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.02f, 0.12f, 1.0f});
                }else{
                        drawSegmentByLineEquation(x1, y1, z1, x2, y2, z2, 20, 0.25f, (Color_RGBA){1.0f, 0.6f, 0.133f, 1.0f});
                }
                step++;
        }
}

void draw(const int step){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        HE_draw(current_object);
        if(step_draw == FALSE){
                switch(option_selected){
                        case 0:
                                draw_adjacent_face_from_face(selected_face);
                                break;
                        case 1:
                                draw_adjacent_face_from_edge(selected_edge);
                                break;
                        case 2:
                                draw_adjacent_face_from_vertex(selected_vertex);
                                break;
                }
        }
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
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
        glWindow = SDL_CreateWindow("OpenGL 3.3", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
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


        int quit = FALSE;
        int counter = 0;

        init();

        char** items = NULL;
        unsigned int list_size = 0;
        update_itemlist(&items, &list_size);

        char** face_list = NULL;
        unsigned int face_list_size = 0;
        update_facelist(&face_list, &face_list_size);
        
        char** edge_list = NULL;
        unsigned int edge_list_size = 0;
        update_edgelist(&edge_list, &edge_list_size);

        ctx = nk_sdl_init(glWindow);
        {struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();}
        while(quit == FALSE){
                input(&quit);
                if(update_item == TRUE){
                        update_item = FALSE;
                        update_itemlist(&items, &list_size);
                        update_facelist(&face_list, &face_list_size);
                        update_edgelist(&edge_list, &edge_list_size);
                        selected_vertex = 0;
                        selected_edge = 0;
                        selected_face = 0;
                }
                // ===== GUI ===================
                if (nk_begin(ctx, "MENU", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)){
                        nk_layout_row_dynamic(ctx, 40, 1);
                        if(nk_button_label(ctx,"Toggle drawing"))
                                step_draw = !step_draw;
                        nk_layout_row_dynamic(ctx, 20, 1);
                        if(step_draw == FALSE){
                                if (nk_option_label(ctx, "Face: Adj faces", option_selected == 0))
                                        option_selected = 0;
                                if (nk_option_label(ctx, "Edge: Adj faces", option_selected == 1)) 
                                        option_selected = 1;
                                if (nk_option_label(ctx, "Vertex: Adj faces", option_selected == 2)) 
                                        option_selected = 2;
                                if (nk_option_label(ctx, "Vertex: Adj edges", option_selected == 3)) 
                                        option_selected = 3;

                                if(option_selected == 0)
                                        selected_face = nk_combo(ctx, (const char* const*)face_list, face_list_size, selected_face, 25, nk_vec2(200,200));
                                if(option_selected == 1)
                                        selected_edge = nk_combo(ctx, (const char* const*)edge_list, edge_list_size, selected_edge, 25, nk_vec2(200,200));
                                if(option_selected == 2 || option_selected == 3)
                                        selected_vertex = nk_combo(ctx, (const char* const*)items, list_size, selected_vertex, 25, nk_vec2(200,200));
                                //if(nk_button_label(ctx,"Vert"));
                        }
                }

                update(&counter);
                nk_end(ctx);

                draw(counter);
                nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
                SDL_GL_SwapWindow(glWindow);
                //quit = TRUE;

        }
        nk_sdl_shutdown();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(glWindow);
        SDL_Quit();
        free(opened_file);
        free(current_object.vertex_array.array);
        free(current_object.face_array.array);
        free(current_object.edge_array.array);
        for(int i = 0; i < face_list_size; i++)
                free(face_list[i]);
        free(face_list);
        for(int i = 0; i < list_size; i++)
                free(items[i]);
        free(items);
        for(int i = 0; i < edge_list_size; i++)
                free(edge_list[i]);
        free(edge_list);
        return 0;
}
