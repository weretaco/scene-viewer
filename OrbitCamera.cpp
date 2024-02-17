#include "OrbitCamera.h"

#include <iostream>

OrbitCamera::OrbitCamera()
: OrbitCamera(0, 0, glm::vec3(0.0f), 0.0f) {
}

OrbitCamera::OrbitCamera(int width, int height, glm::vec3 center, float radius)
: windowWidth(width), windowHeight(height), center(center), up(glm::vec3(0.0f, 0.0f, 1.0f)), radius(radius) {
    pos = glm::vec3(50.0f, 10.0f, 50.0f);
    //pos = glm::vec3(0.0f, 0.0f, radius);

    polarAngle = glm::pi<float>() / 4.0f;
    azimuthAngle = 0.0f;
}

glm::mat4 OrbitCamera::getViewMatrix() {
    return glm::lookAt(getEye(), center, up);
}

void OrbitCamera::startRotate(double x, double y) {
    std::cout << "Starting rotation..." << std::endl;

    startX = x;
    startY = y;
    startPolar = polarAngle;
    startAzimuth = azimuthAngle;
}

void OrbitCamera::rotate(double x, double y) {
    float maxPolar = glm::degrees(glm::pi<float>());
    float minPolar = -glm::degrees(glm::pi<float>());

    float maxAzimuth = glm::degrees(2.0f * glm::pi<float>());
    float minAzimuth = 0.0f;


    double xDiff = x - startX;
    polarAngle = startPolar + (xDiff * (maxPolar - minPolar) / windowHeight);

    if (polarAngle > maxPolar) {
        polarAngle = maxPolar;
    }
    if (polarAngle < minPolar) {
        polarAngle = minPolar;
    }

    double yDiff = y - startY;
    azimuthAngle = startAzimuth + (yDiff * (maxAzimuth - minAzimuth) / windowWidth);
}

glm::vec3 OrbitCamera::getEye() {
    float sinPolar = sin(glm::radians(polarAngle));
    float cosPolar =  cos(glm::radians(polarAngle));
    float sinAzimuth = sin(glm::radians(azimuthAngle));
    float cosAzimuth =  cos(glm::radians(azimuthAngle));

    float x = center.x + radius * cosPolar * cosAzimuth;
    float y = center.y + radius * sinPolar;
    float z = center.z + radius * cosPolar * sinAzimuth;

    return glm::vec3(x, y, z);
}