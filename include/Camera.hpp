#ifndef CAMERA_HPP
#define CAMERA_HPP

class Camera {
public:
    void Update(float playerX, float worldWidth);

    float GetX() const { return m_X; }

private:
    float m_TriggerX = 0.0F;
    float m_X = 0.0F;
};

#endif
