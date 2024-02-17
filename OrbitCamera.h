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
        void startRotate(double x, double y);
        void rotate(double x, double y);
        glm::vec3 getEye();

    private:
        int windowWidth, windowHeight;
        glm::vec3 pos;
        glm::vec3 center;
        glm::vec3 up;
        float azimuthAngle;
        float polarAngle;
        float radius;
        double startX, startY;
        float startPolar, startAzimuth;
};

#endif // _ORBIT_CAMERA_H