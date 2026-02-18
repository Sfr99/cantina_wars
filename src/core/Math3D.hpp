/*
 * Math3D.hpp
 * Tipos matemáticos básicos para 3D: vectores (Vec3, Vec4) y
 * matrices 4x4 (Mat4) con operaciones estándar y factories estáticas.
 */
#pragma once
#include <cmath>

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float t)       const { return {x*t,   y*t,   z*t};   }
    Vec3 operator-()              const { return {-x, -y, -z};           }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    float dot(const Vec3& o)   const { return x*o.x + y*o.y + z*o.z; }
    Vec3  cross(const Vec3& o) const { return {y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x}; }
    float length()             const { return sqrtf(x*x + y*y + z*z); }

    /* Devuelve el vector unitario; retorna (0,0,1) si la longitud es insignificante. */
    Vec3 normalized() const {
        float l = length();
        return (l < 1e-6f) ? Vec3{0,0,1} : Vec3{x/l, y/l, z/l};
    }
};

struct Vec4 { float x=0, y=0, z=0, w=1; };

struct Mat4 {
    float m[4][4] = {};

    /* Multiplica la matriz por un Vec4 y devuelve el vector resultante. */
    Vec4 multiply(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w,
        };
    }

    /* Producto de dos matrices 4x4. */
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }

    static Mat4 identity() {
        Mat4 r; r.m[0][0]=r.m[1][1]=r.m[2][2]=r.m[3][3]=1.f; return r;
    }

    static Mat4 rotationX(float a) {
        Mat4 r = identity();
        r.m[1][1]= cosf(a); r.m[1][2]=-sinf(a);
        r.m[2][1]= sinf(a); r.m[2][2]= cosf(a);
        return r;
    }

    static Mat4 rotationY(float a) {
        Mat4 r = identity();
        r.m[0][0]= cosf(a); r.m[0][2]= sinf(a);
        r.m[2][0]=-sinf(a); r.m[2][2]= cosf(a);
        return r;
    }

    static Mat4 rotationZ(float a) {
        Mat4 r = identity();
        r.m[0][0]= cosf(a); r.m[0][1]=-sinf(a);
        r.m[1][0]= sinf(a); r.m[1][1]= cosf(a);
        return r;
    }

    static Mat4 translation(float x, float y, float z) {
        Mat4 r = identity();
        r.m[0][3]=x; r.m[1][3]=y; r.m[2][3]=z;
        return r;
    }

    static Mat4 scale(float s) {
        Mat4 r = identity(); r.m[0][0]=r.m[1][1]=r.m[2][2]=s; return r;
    }

    /* Construye una vista orientada desde eye hacia target; worldUp no debe ser paralelo a (target-eye). */
    static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 worldUp) {
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(worldUp).normalized();
        Vec3 u = r.cross(f);
        Mat4 mat;
        mat.m[0][0]=r.x; mat.m[0][1]=r.y; mat.m[0][2]=r.z; mat.m[0][3]=-r.dot(eye);
        mat.m[1][0]=u.x; mat.m[1][1]=u.y; mat.m[1][2]=u.z; mat.m[1][3]=-u.dot(eye);
        mat.m[2][0]=f.x; mat.m[2][1]=f.y; mat.m[2][2]=f.z; mat.m[2][3]=-f.dot(eye);
        mat.m[3][0]=0.f; mat.m[3][1]=0.f; mat.m[3][2]=0.f; mat.m[3][3]=1.f;
        return mat;
    }
};