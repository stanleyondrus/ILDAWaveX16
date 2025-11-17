#include "PointRingBuffer.h"

bool PointRingBuffer::canItFit(uint16_t count) {
    taskENTER_CRITICAL(&spinlock);
    uint16_t free_space;
    if (head >= tail) free_space = POINT_BUFFER_SIZE - (head - tail);
    else free_space = tail - head;
    taskEXIT_CRITICAL(&spinlock);
    return count < free_space;
}

bool PointRingBuffer::addPoints(const Point* points, uint16_t num) {
    bool success = true;
    taskENTER_CRITICAL(&spinlock);
    for (uint16_t i = 0; i < num; i++) {
        size_t next = (head + 1) % POINT_BUFFER_SIZE;
        if (next == tail) { 
            success = false;
            break; // buffer full
        }
        buffer[head] = points[i];
        head = next;
    }
    taskEXIT_CRITICAL(&spinlock);
    return success;
}

bool PointRingBuffer::addPoint(const Point& p) { return addPoints(&p, 1); }

bool PointRingBuffer::getPoint(Point& p) {
    if (head == tail) return false;
    taskENTER_CRITICAL(&spinlock);
    p = buffer[tail];
    tail = (tail + 1) % POINT_BUFFER_SIZE;
    taskEXIT_CRITICAL(&spinlock);
    return true;
}

uint16_t PointRingBuffer::getPoints(Point* points, uint16_t max) {
    uint16_t count = 0;
    taskENTER_CRITICAL(&spinlock);
    while (count < max && tail != head) {
        points[count] = buffer[tail];
        tail = (tail + 1) % POINT_BUFFER_SIZE;
        count++;
    }
    taskEXIT_CRITICAL(&spinlock);
    return count;
}

void PointRingBuffer::clear() { head = tail = 0; }