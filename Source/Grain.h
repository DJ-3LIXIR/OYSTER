#pragma once

struct Grain
{
    float position = 0.0f;
    int size = 0;
    float pitch = 1.0f;
    float pan = 0.5f;
    float amplitude = 0.0f;
    int currentSample = 0;
    bool isActive = false;
};
