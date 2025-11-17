#pragma once
#include <Arduino.h>
#include <ILDA.h>

#define POINT_BUFFER_SIZE 8192

class PointRingBuffer {
public:
    bool canItFit(uint16_t count);
    bool addPoints(const Point* points, uint16_t num);
    bool addPoint(const Point& p);
    bool getPoint(Point& p);
    uint16_t getPoints(Point* points, uint16_t max);
    void clear();

private:
    Point buffer[POINT_BUFFER_SIZE];
    volatile size_t head = 0;
    volatile size_t tail = 0;
    portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
};