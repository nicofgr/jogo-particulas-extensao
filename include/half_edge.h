#ifndef HALF_EDGE_H
#define HALF_EDGE_H

#include <stdlib.h>
#include "obj_loader.h"

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

unsigned int* HE_get_object_single_face_as_array(HE_Object object, int* size, const unsigned int face){
        HE_Face_Array faces = object.face_array;
        HE_Edge_Array edges = object.edge_array;
        int counter = 0;

        int starting_edge = faces.array[face].edge_ID;
        int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
        counter++;
        int next_edge = edges.array[starting_edge].nextEdge_ID;
        int next_vertex = edges.array[next_edge].origin_vertex_ID;
        
        while(starting_edge != next_edge){
                next_edge = edges.array[next_edge].nextEdge_ID;
                next_vertex = edges.array[next_edge].origin_vertex_ID;
                counter++;
        }


        unsigned int* output = (unsigned int*)malloc(sizeof(unsigned int)*counter);
        counter = 0;

        starting_edge = faces.array[face].edge_ID;
        starting_vertex = edges.array[starting_edge].origin_vertex_ID;
        output[counter] = starting_vertex;
        counter++;
        next_edge = edges.array[starting_edge].nextEdge_ID;
        next_vertex = edges.array[next_edge].origin_vertex_ID;
        
        while(starting_edge != next_edge){
                output[counter] = next_vertex;
                next_edge = edges.array[next_edge].nextEdge_ID;
                next_vertex = edges.array[next_edge].origin_vertex_ID;
                counter++;
        }
        *size = counter;
        return output;
}

unsigned int* HE_get_object_faces_as_array(HE_Object object, int* size){
        HE_Face_Array faces = object.face_array;
        HE_Edge_Array edges = object.edge_array;
        int counter = 0;
        for(int i = 0; i < faces.size; i++){
                int starting_edge = faces.array[i].edge_ID;
                int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
                counter++;
                int next_edge = edges.array[starting_edge].nextEdge_ID;
                int next_vertex = edges.array[next_edge].origin_vertex_ID;
                
                while(starting_edge != next_edge){
                        next_edge = edges.array[next_edge].nextEdge_ID;
                        next_vertex = edges.array[next_edge].origin_vertex_ID;
                        counter++;
                }
        }
        unsigned int* output = (unsigned int*)malloc(sizeof(unsigned int)*counter);
        counter = 0;
        for(int i = 0; i < faces.size; i++){
                int starting_edge = faces.array[i].edge_ID;
                int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
                output[counter] = starting_vertex;
                counter++;
                int next_edge = edges.array[starting_edge].nextEdge_ID;
                int next_vertex = edges.array[next_edge].origin_vertex_ID;
                
                while(starting_edge != next_edge){
                        output[counter] = next_vertex;
                        next_edge = edges.array[next_edge].nextEdge_ID;
                        next_vertex = edges.array[next_edge].origin_vertex_ID;
                        counter++;
                }
        }
        *size = counter;
        return output;
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

#endif
