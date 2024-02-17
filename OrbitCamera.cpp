#include "OrbitCamera.h"

#include <iostream>

OrbitCamera::OrbitCamera()
: OrbitCamera(0, 0, glm::vec3(0.0f), 0.0f) {
}

OrbitCamera::OrbitCamera(int width, int height, glm::vec3 center, float radius)
: windowWidth(width), windowHeight(height), center(center), up(glm::vec3(0.0f, 0.0f, 1.0f)),
radius(radius), minRadius(radius / 10.0f) {
    polarAngle = glm::pi<float>() / 4.0f;
    azimuthAngle = 0.0f;
}

glm::mat4 OrbitCamera::getViewMatrix() {
    return glm::lookAt(getEye(), center, up);
}

void OrbitCamera::startRotate(float x, float y) {
    startX = x;
    startY = y;
    startPolar = polarAngle;
    startAzimuth = azimuthAngle;
}

void OrbitCamera::rotate(float x, float y) {
    float fullCircle = glm::degrees(2.0f * glm::pi<float>());
    float maxAzimuth = fullCircle;
    float minAzimuth = 0.0f;

    float xDiff = x - startX;
    azimuthAngle = startAzimuth + (xDiff * (maxAzimuth - minAzimuth) / windowWidth);
    azimuthAngle = fmodf(azimuthAngle, fullCircle);

    if (azimuthAngle < minAzimuth) {
        azimuthAngle += fullCircle;
    }

    float maxPolar = glm::degrees(glm::pi<float>()) / 2.0f - 0.001f;
    float minPolar = -maxPolar;

    float yDiff = y - startY;
    polarAngle = startPolar + (yDiff / windowHeight * (maxPolar - minPolar));

    if (polarAngle > maxPolar) {
        polarAngle = maxPolar;
    }
    if (polarAngle < minPolar) {
        polarAngle = minPolar;
    }
}

void OrbitCamera::changeZoom(float changeDir) {
    radius = std::max(radius + changeDir, minRadius);
}

glm::vec3 OrbitCamera::getEye() {
    float sinPolar = sin(glm::radians(polarAngle));
    float cosPolar =  cos(glm::radians(polarAngle));
    float sinAzimuth = sin(glm::radians(azimuthAngle));
    float cosAzimuth =  cos(glm::radians(azimuthAngle));

    float x = center.x + radius * cosPolar * cosAzimuth;
    float y = center.y + radius * cosPolar * sinAzimuth;
    float z = center.z + radius * sinPolar;

    return glm::vec3(x, y, z);
}