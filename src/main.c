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
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // for wait time
#include <cglm/cglm.h>

#define SCREEN_WIDTH   600
#define SCREEN_HEIGHT  600
#define TRUE  1
#define FALSE 0
#define TARGET_FPS 60
#define FRAME_TARGET_TIME 1000/TARGET_FPS

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
                //printf("%.2f %.2f %.2f\n", x, y, z);
                verts[i*3 + 0] = x;
                verts[i*3 + 1] = y;
                verts[i*3 + 2] = z;
                x += step*a;
                y += step*b;
                z += step*c;
        }
        //printf("\n");

        mat4 model;
        glm_mat4_identity(model);
        //glm_scale(model, (vec4){0.1f, 0.1f, 0.1f, 1.0f});
        //glm_translate(model, (vec3){0.0f, -20.0f, 0.0f});
        glm_rotate(model, (float)SDL_GetTicks()/2000.0f, (vec3){0.0f, 1.0f, 0.0f});

        mat4 view;
        glm_mat4_identity(view);
        //glm_translate(view, (vec3){0.0f, 0.0f, -7.0f});

        vec3 target_dir;

        vec3 direction;
        direction[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
        direction[1] = sin(glm_rad(pitch));
        direction[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
        glm_normalize_to(direction, camera_front);
        glm_vec3_add(camera_pos, camera_front, target_dir);
        glm_lookat(camera_pos, target_dir, camera_up, view);

        mat4 proj;
        glm_perspective(glm_rad(45), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f, proj);

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



typedef struct Vertex{
        float x;
        float y;
        float z;
}Vertex;

typedef struct VertexArray{
        Vertex* verts;
        unsigned int size;
}VertexArray;

void vertexArray_Push(VertexArray* verArray, float x, float y, float z){
        if(verArray->size == 0){
                verArray->verts = (Vertex*)malloc(sizeof(Vertex));
        }else{
                verArray->verts = (Vertex*)realloc(verArray->verts, sizeof(Vertex)*(verArray->size+1));
        }
        verArray->verts[verArray->size].x = x;
        verArray->verts[verArray->size].y = y;
        verArray->verts[verArray->size].z = z;
        verArray->size++;
}

// HALF-EDGE IMPLEMENTATION, extract later
struct HE_Face;
struct HE_HalfEdge;
struct HE_Vertex;

typedef struct HE_Vertex{
        float x;
        float y;
        float z;
        unsigned int inc_edge_ID;
}HE_Vertex;

typedef struct HE_Face{
        unsigned int edge_ID;
}HE_Face;

typedef struct HE_HalfEdge{
        unsigned int origin_vertex_ID;
        unsigned int twin_edge_ID;
        unsigned int inc_face_ID;
        unsigned int prvsEdge_ID;
        unsigned int nextEdge_ID;
}HE_HalfEdge;

typedef struct HE_Vertex_Array{
        HE_Vertex* array;
        unsigned int size;
}HE_Vertex_Array;

typedef struct HE_Face_Array{
        HE_Face* array;
        unsigned int size;
}HE_Face_Array;

typedef struct HE_Edge_Array{
        HE_HalfEdge* array;
        unsigned int size;
}HE_Edge_Array;

typedef struct{
        HE_Vertex_Array vertex_array;
        HE_Face_Array face_array;
        HE_Edge_Array edge_array;
}HE_Object;

void HE_vertexArray_Push(HE_Vertex_Array* verArray, float x, float y, float z, unsigned int inc_edge_ID){
        if(verArray->size == 0){
                verArray->array = (HE_Vertex*)malloc(sizeof(HE_Vertex));
        }else{
                verArray->array = (HE_Vertex*)realloc(verArray->array, sizeof(HE_Vertex)*(verArray->size+1));
        }
        verArray->array[verArray->size].x = x;
        verArray->array[verArray->size].y = y;
        verArray->array[verArray->size].z = z;
        verArray->array[verArray->size].inc_edge_ID = inc_edge_ID;
        verArray->size++;
}

void HE_edgeArray_Push(HE_Edge_Array* edgeArray, unsigned int origin_vertex_ID, unsigned int twin_ID, unsigned int inc_face_ID, unsigned int nextEdge_ID, unsigned int prvsEdge_ID){
        if(edgeArray->size == 0){
                edgeArray->array = (HE_HalfEdge*)malloc(sizeof(HE_HalfEdge));
        }else{
                edgeArray->array = (HE_HalfEdge*)realloc(edgeArray->array, sizeof(HE_HalfEdge)*(edgeArray->size+1));
        }
        edgeArray->array[edgeArray->size].origin_vertex_ID = origin_vertex_ID;
        edgeArray->array[edgeArray->size].twin_edge_ID = twin_ID;
        edgeArray->array[edgeArray->size].inc_face_ID = inc_face_ID;
        edgeArray->array[edgeArray->size].prvsEdge_ID = prvsEdge_ID;
        edgeArray->array[edgeArray->size].nextEdge_ID = nextEdge_ID;
        edgeArray->size++;
}

void HE_faceArray_Push(HE_Face_Array* faceArray, unsigned int edge_ID){
        if(faceArray->size == 0){
                faceArray->array = (HE_Face*)malloc(sizeof(HE_Face));
        }else{
                faceArray->array = (HE_Face*)realloc(faceArray->array, sizeof(HE_Face)*(faceArray->size+1));
        }
        faceArray->array[faceArray->size].edge_ID = edge_ID;
        faceArray->size++;
}

HE_Object HE_load(const char* filename);
HE_Object current_object;

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


        glBindVertexArray(VAO);
        /**
          glBindBuffer(GL_ARRAY_BUFFER, VBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
         **/

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        current_object = HE_load("test.obj");
}
char* opened_file = NULL;

void input(int * quit){
        SDL_Event e;
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
                                SDL_free(e.drop.file);
                                break;
                }
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
}

int last_frame_time = 0;
int lastTime = 0;

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

typedef struct{
        unsigned int* vertex_ID;
        unsigned int size;
}Face;

typedef struct{
        Face* array;
        unsigned int size;
}FaceArray;

typedef struct{
        VertexArray vertex;
        FaceArray   face;
} OBJ;

void face_array_push(FaceArray* faceArray, const Face face){
        if(faceArray->size == 0){
                faceArray->array = (Face*)malloc(sizeof(Face));
        }else{
                faceArray->array = (Face*)realloc(faceArray->array, sizeof(Face)*(faceArray->size+1));
        }
        faceArray->array[faceArray->size] = face;
        faceArray->size++;
}

void face_push(Face* face, unsigned int vertex_ID){
        if(face->size == 0){
                face->vertex_ID = (unsigned int*)malloc(sizeof(unsigned int));
        }else{
                face->vertex_ID = (unsigned int*)realloc(face->vertex_ID, sizeof(unsigned int)*(face->size+1));
        }
        face->vertex_ID[face->size] = vertex_ID;
        face->size++;
}

void read_obj(const char* filename, OBJ* objct){
        FILE* file = fopen(filename, "r");
        if(file == NULL){
                printf("File %s could not be opened\n", filename);
                exit(1);
        }

        VertexArray vertices = {NULL, 0};
        FaceArray faces = {NULL, 0};
        
        int ch;
        while(1){
                float x, y, z, w = 1;
                unsigned int v;
                Face face = {NULL, 0};
                int end;
                ch = fgetc(file);
                if(ch == EOF)
                        break;
                switch(ch){
                        case '#':
                                while(ch != '\n')
                                        ch = fgetc(file);
                                break;
                        case 'v':
                                fscanf(file, "%f %f %f %f", &x, &y ,&z, &w);
                                //printf("v %.2f %.2f %.2f %.2f\n", x, y, z, w);
                                vertexArray_Push(&vertices, x, y, z);
                                break;
                        case 'f':
                                end = FALSE;
                                while(end == FALSE){
                                        while(fscanf(file, "%u", &v) == 1){
                                                face_push(&face, v);
                                                //printf("%u ", v);
                                        }  // Parou no \n ou no '/'
                                        fseek(file, -1, SEEK_CUR);
                                        int aux = fgetc(file);
                                        while(1){
                                                if(aux == '\n'){
                                                        end = TRUE;
                                                        break;
                                                }
                                                if(aux == ' '){
                                                        break;
                                                }
                                                aux = fgetc(file);
                                        }
                                }
                                //fscanf(file, "%u", &v);
                                //printf("%u ", v);
                                //printf("\n");
                                face_array_push(&faces, face);
                                break;
                        default:
                                break;
                }
        }
        if(1){
        printf("\n");
        printf("STRUCTURES\n");
        for(int i = 0; i < vertices.size; i++){
                float x = vertices.verts[i].x;
                float y = vertices.verts[i].y;
                float z = vertices.verts[i].z;
                printf("v %.2f %.2f %.2f\n", x, y, z);
        }

        printf("Faces %d\n", faces.size);
        for(int i = 0; i < faces.size; i++){
                printf("f ");
                for(int j = 0; j < faces.array[i].size; j++){
                        printf("%u ", faces.array[i].vertex_ID[j]);
                }
                printf("\n");
        }
        }
        
        objct->face   = faces;
        objct->vertex = vertices;

        fclose(file);
}

void free_obj(OBJ object){
        FaceArray faces      = object.face;
        VertexArray vertices = object.vertex;
        for(int i = 0; i < faces.size; i++){
                free(faces.array[i].vertex_ID);
        }
        free(vertices.verts);
        free(faces.array);
}

void HE_draw(const HE_Object object){
        HE_Edge_Array   edgeArray = object.edge_array;
        HE_Vertex_Array verArray  = object.vertex_array;
        HE_Face_Array   faceArray = object.face_array;

        float scds = 10;  // Time to draw whole figure
        float cnt = (int)(SDL_GetTicks()/((scds*1000.0f)/edgeArray.size))%(edgeArray.size) + 1;
        int step = 0;

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
                if (step == cnt-1)
                        drawSegmentByLineEquation(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){1.0f, 0.0f, 0.0f, 1.0f});
                drawSegmentByLineEquation(x1, y1, z1, x2, y2, z2, 20, 0.25f, (Color_RGBA){1.0f, 1.0f, 0.0f, 1.0f});
                step++;
        }
}

HE_Object HE_load(const char* filename){
        float x, y, z;
        OBJ object;
        read_obj(filename, &object);

        HE_Vertex_Array verArray  = {NULL, 0};
        HE_Face_Array   faceArray = {NULL, 0};
        HE_Edge_Array   edgeArray = {NULL, 0};

        // Carrega os vértices
        for(int i = 0; i < object.vertex.size; i++){
                x = object.vertex.verts[i].x;
                y = object.vertex.verts[i].y;
                z = object.vertex.verts[i].z;
                HE_vertexArray_Push(&verArray, x, y, z, 0);
        }
        // Centering model in local space
        float sumX = 0;
        float sumY = 0;
        float sumZ = 0;
        for(int i = 0; i < verArray.size; i++){
                sumX += verArray.array[i].x; 
                sumY += verArray.array[i].y; 
                sumZ += verArray.array[i].z; 
        }
        for(int i = 0; i < verArray.size; i++){
                verArray.array[i].x -= (sumX/verArray.size);
                verArray.array[i].y -= (sumY/verArray.size);
                verArray.array[i].z -= (sumZ/verArray.size);
        }

        // -----------
        int conMap [verArray.size][verArray.size];

        for(int i = 0; i < verArray.size; i++)
                for(int j = 0; j < verArray.size; j++)
                        conMap[i][j] = -1;


        int* verts;
        int edge_counter = 0;
        for(int i = 0; i < object.face.size; i++){
                // Getting verts from this face
                int face_size = object.face.array[i].size; // How many verts on this face
                verts = (int*) malloc(sizeof(int)*face_size);
                for(int j = 0; j < face_size; j++){
                        verts[j] = object.face.array[i].vertex_ID[j];
                }
                for(int j = 0; j < face_size; j++){
                        conMap[verts[j]-1][verts[(j+1)%face_size]-1] = edge_counter++;
                }

                // Making new face with verts
                HE_faceArray_Push(&faceArray, edgeArray.size);
                int index = edgeArray.size;
                for(int j = 0; j < face_size; j++){
                        verArray.array[verts[j]-1].inc_edge_ID = index+j;
                }

                for(int j = 0; j < face_size; j++){
                        HE_edgeArray_Push(&edgeArray, verts[j]-1, 0, faceArray.size-1, index+((j+1)%face_size), index+((j+2)%face_size));
                }
                free(verts);
        }

        // Determina os twins dos edges a partir do mapa de relações
        // Caso não tenha twin, cria um edge novo
        for(int index = 0; index < edgeArray.size; index++){
                HE_HalfEdge edgeData = edgeArray.array[index];
                for(int i = 0; i < verArray.size; i++){
                        for(int j = 0; j < verArray.size; j++){
                                if(conMap[i][j] != index)
                                        continue;
                                if(conMap[j][i] == -1){
                                        HE_edgeArray_Push(&edgeArray, j, index, -1, -1, -1);
                                        conMap[j][i] = edgeArray.size-1;
                                }
                                edgeArray.array[index].twin_edge_ID = conMap[j][i];
                        }
                }
        }

        // Percorre os edges por relações para encontrar o previous e o next
        for(int index = 0; index < edgeArray.size; index++){
                HE_HalfEdge edgeData = edgeArray.array[index];
                HE_HalfEdge* array = edgeArray.array;
                if(edgeData.nextEdge_ID != -1)
                        continue;
                edgeArray.array[index].prvsEdge_ID = array[array[array[array[array[index].twin_edge_ID].nextEdge_ID].twin_edge_ID].nextEdge_ID].twin_edge_ID;
                edgeArray.array[index].nextEdge_ID = array[array[array[array[array[index].twin_edge_ID].prvsEdge_ID].twin_edge_ID].prvsEdge_ID].twin_edge_ID;
        }

        free_obj(object);
        HE_Object ret;
        ret.edge_array   = edgeArray;
        ret.vertex_array = verArray;
        ret.face_array   = faceArray;
        return ret;
}


void draw(const int step){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        HE_draw(current_object);
        SDL_GL_SwapWindow(glWindow);

        mat4 proj;
        glm_perspective(glm_rad(45), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f, proj);
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


        init();
        int quit = FALSE;
        int counter = 0;
        while(quit == FALSE){
                input(&quit);
                update(&counter);
                draw(counter);
                //quit = TRUE;
        }
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(glWindow);
        SDL_Quit();
        free(opened_file);
        free(current_object.vertex_array.array);
        free(current_object.face_array.array);
        free(current_object.edge_array.array);
        return 0;
}
