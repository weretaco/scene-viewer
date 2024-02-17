#ifndef _ORBIT_CAMERA_H
#define _ORBIT_CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OrbitCamera {
    public:
        OrbitCamera();
        OrbitCamera(int width, int height, glm::vec3 center, float radius);

        void setWindowSize(int width, int height);
        glm::mat4 getViewMatrix();
        void startRotate(float x, float y);
        void rotate(float x, float y);
        void changeZoom(float changeDir);
        glm::vec3 getEye();

    private:
        int windowWidth, windowHeight;
        glm::vec3 center;
        glm::vec3 up;
        float azimuthAngle;
        float polarAngle;
        float radius;
        float minRadius;
        float startX, startY;
        float startPolar, startAzimuth;
};

#endif // _ORBIT_CAMERA_H