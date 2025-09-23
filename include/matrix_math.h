#ifndef MATRIX_MATH_H
#define MATRIX_MATH_H

#include <math.h>
#define MM_PI 3.14

typedef float vec3[3];
typedef float vec4[4];
typedef float mat4[4][4];

void mm_mat4_identity(mat4 mat){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        mat[i][j] = 0;
                        if(i == j)
                                mat[i][j] = 1;
                }
        }
}

void mm_scale(mat4 mat, vec4 vec){
}
void mm_translate(mat4 mat, vec3 vec){
}
void mm_rotate(mat4 mat, float radians, vec3 vec){
}

float mm_rad(float degrees){
        return degrees*0.0174533;
}

void mm_normalize_to(vec3 source, vec3 dest){
        float den = sqrt(pow(source[0],2) + pow(source[1],2) + pow(source[2],2));
        dest[0] = source[0]/den;
        dest[1] = source[1]/den;
        dest[2] = source[2]/den;
}
void mm_vec3_add(vec3 a, vec3 b, vec3 dest){
        dest[0] = a[0] + b[0];
        dest[1] = a[1] + b[1];
        dest[2] = a[2] + b[2];
}

void mm_vec3_sub(vec3 a, vec3 b, vec3 dest){
        dest[0] = a[0] - b[0];
        dest[1] = a[1] - b[1];
        dest[2] = a[2] - b[2];
}

void mm_cross(vec3 a, vec3 b, vec3 dest){
        dest[0] = a[1]*b[2] - b[1]*a[2];
        dest[1] = a[2]*b[0] - b[2]*a[0]; 
        dest[2] = a[0]*b[1] - b[0]*a[1];
}

void mm_mat4_mul(mat4 a, mat4 b, mat4 dest){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        float sum = 0;
                        for(int k = 0; k < 4; k++){
                                sum += (a[i][k] * b[k][j]);
                        }
                        dest[i][j] = sum;
                }
        }
}

void mm_lookat(vec3 eye_pos, vec3 target, vec3 up, mat4 dest){
        //target = (vec3){0.0f, 0.0f, 0.0f};
        vec3 front_axis; // Direction vector
        mm_vec3_sub(eye_pos, target, front_axis);
        mm_normalize_to(front_axis, front_axis);
        vec3 right_axis;
        mm_cross(up, front_axis, right_axis);
        //mm_cross(front_axis, up, right_axis);
        mm_normalize_to(right_axis, right_axis);
        vec3 up_axis;
        mm_cross(front_axis, right_axis, up_axis);
        //mm_cross(right_axis, front_axis, up_axis);
        mm_normalize_to(up_axis, up_axis);

        mat4 mat_left;
        mm_mat4_identity(mat_left);
        for(int i = 0; i < 3; i++){
                mat_left[i][0] = right_axis[i];
                mat_left[i][1] = up_axis[i];
                mat_left[i][2] = front_axis[i];
        }

        mat4 mat_right;
        mm_mat4_identity(mat_right);
        for(int i = 0; i < 3; i++){
                mat_right[3][i] = -eye_pos[i];
        }
        mm_mat4_mul(mat_right, mat_left, dest);
}

void mm_perspective(float fov, float aspect_ratio, float near, float far, mat4 dest);

#endif
