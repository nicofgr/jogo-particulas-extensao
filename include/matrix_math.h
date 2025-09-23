#ifndef MATRIX_MATH_H
#define MATRIX_MATH_H

#include <math.h>
#include <stddef.h>
#define MM_PI 3.14

// NEED TO CHANGE TO COLUMN MAJOR AS THIS IS WHAT OPENGL USES

typedef float vec3[3];
typedef float vec4[4];
typedef float mat4[4][4];

void print_mat4(mat4 mat){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        printf("%.2f ", mat[i][j]);
                }
                printf("\n");
        }
}
void mm_mat4_identity(mat4 mat){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        mat[i][j] = 0;
                        if(i == j)
                                mat[i][j] = 1;
                }
        }
}

void mm_mat4_copy(const mat4 source, mat4 dest){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        dest[i][j] = source[i][j];
                }
        }
}

void mm_mat4_mul(const mat4 a, const mat4 b, mat4 dest){  // AB[x]
        mat4 result;
        mm_mat4_identity(result);
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        float sum = 0;
                        for(int k = 0; k < 4; k++){
                                sum += (a[i][k] * b[k][j]);
                        }
                        result[i][j] = sum;
                }
        }
        mm_mat4_copy(result, dest);
}

void mm_mat4_transpose(mat4 mat){
        for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                        if(j > i){
                                float temp;
                                temp = mat[i][j];
                                mat[i][j] = mat[j][i];
                                mat[j][i] = temp;
                        }
                }
        }
}
void mm_scale(mat4 mat, vec3 vec){
        mat4 scaler;
        mm_mat4_identity(scaler);
        for(int i = 0; i < 3; i++){
                scaler[i][i] = vec[i];
        }
        mm_mat4_mul(scaler, mat, mat);
}
void mm_translate(mat4 mat, vec3 vec){
        mat4 translater;
        mm_mat4_identity(translater);
        for(int i = 0; i < 3; i++){
                translater[i][3] = vec[i];
        }
        mm_mat4_transpose(translater); // CHANGE THIS IF CHANGE TO COLUMN-MAJOR
        mm_mat4_mul(translater, mat, mat);
}

float mm_vec3_mag(const vec3 vec){
        return sqrt(pow(vec[0],2) + pow(vec[1],2) + pow(vec[2],2));
}

void mm_rotate_around_y(mat4 mat, float radians){
        mat4 ry;
        mm_mat4_identity(ry);
        ry[0][0] = cos(radians);
        ry[0][2] = sin(radians);
        ry[2][0] = -sin(radians);
        ry[2][2] = cos(radians);
        mm_mat4_transpose(ry);
        mm_mat4_mul(ry, mat, mat);
}

void mm_rotate_around_x(mat4 mat, float radians){
        mat4 rx;
        mm_mat4_identity(rx);
        rx[1][1] = cos(radians);
        rx[1][2] = -sin(radians);
        rx[2][1] = sin(radians);
        rx[2][2] = cos(radians);
        mm_mat4_transpose(rx);
        mm_mat4_mul(rx, mat, mat);
}

void mm_rotate(mat4 mat, float radians, vec3 axis){
        // Find angle theta between axis and x (dot prod)
        // Project axis on zy plane (sin(theta))
        // Find angle phi between axis and -z (dot prod)
        // Rotate everything by theta then phi
        mat4 rz;
        mm_mat4_identity(rz);
        rz[0][0] = cos(radians);
        rz[0][1] = -sin(radians);
        rz[1][0] = sin(radians);
        rz[1][1] = cos(radians);
        mm_mat4_transpose(rz);

        float theta = acos(axis[1]/mm_vec3_mag(axis));

        vec3 axis_xz = {axis[0], 0, axis[2]}; // proj of axis on xz plane
        float phi = acos(axis_xz[2]/mm_vec3_mag(axis_xz));
        if(theta == 0)
                phi = 0;

        mm_rotate_around_y(mat, phi);
        mm_rotate_around_x(mat, theta);
        mm_rotate_around_y(mat, radians);
        mm_rotate_around_x(mat, -theta);
        mm_rotate_around_y(mat, -phi);
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

void mm_cross(const vec3 a, const vec3 b, vec3 dest){
        dest[0] = a[1]*b[2] - b[1]*a[2];
        dest[1] = a[2]*b[0] - b[2]*a[0]; 
        dest[2] = a[0]*b[1] - b[0]*a[1];
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


void mm_ortho(float left, float right, float bottom, float top, float near, float far, mat4 dest){
        mm_mat4_identity(dest);
        // Translation
        mat4 translation;
        mm_mat4_identity(translation);
        translation[0][3] = -(right+left)/2;
        translation[1][3] = -(bottom+top)/2;
        translation[2][3] = -(near+far)/2;
        // Scale
        mat4 scale;
        mm_mat4_identity(scale);
        scale[0][0] = 2/(right-left);
        scale[1][1] = 2/(top-bottom);
        scale[2][2] = -2/(far-near);  // Minus sign because looks down -z axis
        //Result
        mm_mat4_mul(scale, translation, dest);
        dest[2][3] *= -1;  // Because near and far are at negative Z in camera space
                           // I could also have just negated 'near' and 'far' at the start
                           // I'm receiving positive numbers but they should be negative
        mm_mat4_transpose(dest);  // Transposing because opengl use column-major
}

void mm_perspective(float fov, float aspect_ratio, float near, float far, mat4 dest){
        float m1 = far+near;
        float m2 = -(far*near);
        mat4 pers;
        mm_mat4_identity(pers);
        pers[0][0] = near;
        pers[1][1] = near;
        pers[2][2] = m1;
        pers[2][3] = m2;
        pers[3][2] = 1;
        pers[3][3] = 0;

        float bottom = near*tan(fov/2);
        float right = near*aspect_ratio*tan(fov/2);

        mat4 ortho;
        mm_mat4_identity(ortho);
        mm_ortho(-right,right, -bottom,bottom,near, far, ortho);
        mm_mat4_transpose(ortho);
        ortho[2][3] *= -1;  // Undoing this step I made in ortho function
                            // If fix minus in near and far might need to change this

        mm_mat4_mul(ortho, pers, dest);
        dest[2][3] *= -1;
        dest[3][2] *= -1;
        mm_mat4_transpose(dest);
}

void mm_vec3_copy(const vec3 source, vec3 dest){
        dest[0] = source[0];
        dest[1] = source[1];
        dest[2] = source[2];
}
void mm_vec3_muladds(const vec3 a, float scalar, vec3 dest){ //pos += (front*spd)
        dest[0] += a[0]*scalar;
        dest[1] += a[1]*scalar;
        dest[2] += a[2]*scalar;
}
void mm_vec3_crossn(const vec3 a, const vec3 b, vec3 output){
        mm_cross(a, b, output);
        mm_normalize_to(output,output);
}

#endif
