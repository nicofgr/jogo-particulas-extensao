#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <stdlib.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

typedef struct Vertex{
        float x;
        float y;
        float z;
}Vertex;

typedef struct VertexArray{
        Vertex* verts;
        unsigned int size;
}VertexArray;

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
        if(0){
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


#endif
