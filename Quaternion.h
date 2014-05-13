#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "Angel.h"
#include "vec.h"

const float TOLERANCE = 0.00001f;

class Quaternion {
    float x;
    float y;
    float z;
    float w;

    public: 
    Quaternion(float x, float y, float z, float w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    Quaternion() {
        this->x = 0;
        this->y = 0;
        this->z = 0;
        this->w = 0;
    }

    // normalising a quaternion works similar to a vector. This method will not do anything
    // if the quaternion is close enough to being unit-length. define TOLERANCE as something
    // small like 0.00001f to get accurate results
    void normalise()
    {
        // Don't normalize if we don't have to
        float mag2 = w * w + x * x + y * y + z * z;
        if (fabs(mag2) > TOLERANCE && fabs(mag2 - 1.0f) > TOLERANCE) {
            float mag = sqrt(mag2);
            w /= mag;
            x /= mag;
            y /= mag;
            z /= mag;
        }
    }
    // We need to get the inverse of a quaternion to properly apply a quaternion-rotation to a vector
    // The conjugate of a quaternion is the same as the inverse, as long as the quaternion is unit-length
    Quaternion getConjugate() const
    {
        return Quaternion(-x, -y, -z, w);
    }
    // Multiplying q1 with q2 applies the rotation q2 to q1
    Quaternion operator* (const Quaternion &rq) const
    {
        // the constructor takes its arguments as (x, y, z, w)
        return Quaternion(w * rq.x + x * rq.w + y * rq.z - z * rq.y,
                w * rq.y + y * rq.w + z * rq.x - x * rq.z,
                w * rq.z + z * rq.w + x * rq.y - y * rq.x,
                w * rq.w - x * rq.x - y * rq.y - z * rq.z);
    }
    // Multiplying a quaternion q with a vector v applies the q-rotation to v
    vec3 operator* (const vec3 &vec) const
    {
        vec3 vn = normalize(vec);

        Quaternion vecQuat, resQuat;
        vecQuat.x = vn.x;
        vecQuat.y = vn.y;
        vecQuat.z = vn.z;
        vecQuat.w = 0.0f;

        resQuat = vecQuat * getConjugate();
        resQuat = *this * resQuat;

        return (vec3(resQuat.x, resQuat.y, resQuat.z));
    }
    // Convert from Axis Angle
    void FromAxis(const vec3 &v, float angle)
    {
        float sinAngle;
        angle *= 0.5f;
        vec3 vn = normalize(v);

        sinAngle = sin(angle);

        x = (vn.x * sinAngle);
        y = (vn.y * sinAngle);
        z = (vn.z * sinAngle);
        w = cos(angle);
    }
    // Convert from Euler Angles
    void FromEuler(float pitch, float yaw, float roll)
    {
        // Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
        // and multiply those together.
        // the calculation below does the same, just shorter

        float p = pitch * M_PI / 360.0;
        float y = yaw * M_PI / 360.0;
        float r = roll * M_PI / 360.0; 
        float sinp = sin(p);
        float siny = sin(y);
        float sinr = sin(r);
        float cosp = cos(p);
        float cosy = cos(y);
        float cosr = cos(r);

        this->x = sinr * cosp * cosy - cosr * sinp * siny;
        this->y = cosr * sinp * cosy + sinr * cosp * siny;
        this->z = cosr * cosp * siny - sinr * sinp * cosy;
        this->w = cosr * cosp * cosy + sinr * sinp * siny;

        normalise();
    }

    // Convert to Matrix
    mat4 getMatrix() const
    {
        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        // This calculation would be a lot more complicated for non-unit length quaternions
        // Note: The constructor of mat4 expects the Matrix in column-major format like expected by
        //   OpenGL
        return mat4( 1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
                2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
                2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Convert to Axis/Angles
    void getAxisAngle(vec3 *axis, float *angle)
    {
        float scale = sqrt(x * x + y * y + z * z);
        axis->x = x / scale;
        axis->y = y / scale;
        axis->z = z / scale;
        *angle = acos(w) * 2.0f;
    }
};
#endif
