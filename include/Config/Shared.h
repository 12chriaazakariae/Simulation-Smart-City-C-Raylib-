#ifndef SHARED_H
#define SHARED_H

#include "raylib.h"
#include <deque>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

// ============================================================================
// GLOBAL CONFIGURATION
// ============================================================================
namespace Config
{
    const int SCREEN_WIDTH = 2400;
    const int SCREEN_HEIGHT = 950;
    const int NUM_SLOTS = 6;
    const float CHARGING_TIME = 45.0f;
    const float SPAWN_RATE = 2.5f;

    // SPEEDS
    const float CAR_SPEED_STATION = 2.0f;
    const float CAR_SPEED_ROAD = 3.5f;
    const float TURN_SPEED = 3.0f;
    const float MOVE_DELAY = 1.0f;

    // DIMENSIONS
    const float CAR_WIDTH = 40.0f;
    const float CAR_HEIGHT = 64.0f;

    // COORDINATES
    const float TOP_LANE_Y = 60.0f;
    const float CHARGING_SLOTS_Y = 140.0f;
    const float WAITING_ROW1_Y = 320.0f;
    const float WAITING_ROW2_Y = 500.0f;
    const float STATION_BOTTOM_Y = 650.0f;
    const float EXIT_LANE_X = 1100.0f;
    const float SLOT_WIDTH = 80.0f;
    const float SLOT_HEIGHT = 90.0f;
    const float SLOT_GAP_X = 40.0f;
    const float FIRST_SLOT_X = 150.0f;
    const float MAIN_ROAD_Y = 850.0f;
    const float RAMP_ENTRY_X = 100.0f;

    const float PARKING_OFFSET_X = 1200.0f;
    const float SPAWN_X = -200.0f;
}

// ============================================================================
// DESIGN COLORS
// ============================================================================
namespace Colors
{
    const Color GRASS = {65, 152, 10, 255};
    const Color ASPHALT = {40, 40, 40, 255};
    const Color CURB = {180, 180, 180, 255};
    const Color LINES = {240, 240, 240, 255};
    const Color PARKING_BG = {50, 50, 50, 255};
    const Color STOPPER = {230, 180, 0, 255};
}

// ============================================================================
// UTILS
// ============================================================================
inline float GetAngleDiff(float current, float target)
{
    float diff = target - current;
    while (diff > 180.0f)
        diff -= 360.0f;
    while (diff < -180.0f)
        diff += 360.0f;
    return diff;
}

// HELPER: Draws rounded corners for fluid roads
// quadrant: 0=BottomRight, 1=BottomLeft, 2=TopLeft, 3=TopRight
inline void DrawRoadCurve(float cx, float cy, float radius, int quadrant)
{
    Vector2 center;
    float startAngle = 0, endAngle = 0;

    if (quadrant == 0)
    { // Top-Left Curve
        center = {cx - radius, cy - radius};
        startAngle = 0;
        endAngle = 90;
    }
    else if (quadrant == 1)
    { // Top-Right Curve
        center = {cx + radius, cy - radius};
        startAngle = 90;
        endAngle = 180;
    }
    else if (quadrant == 2)
    { // Bottom-Right Curve
        center = {cx + radius, cy + radius};
        startAngle = 180;
        endAngle = 270;
    }
    else if (quadrant == 3)
    { // Bottom-Left Curve
        center = {cx - radius, cy + radius};
        startAngle = 270;
        endAngle = 360;
    }

    DrawRing(center, radius - 1.5f, radius + 1.5f, startAngle, endAngle, 32, Colors::LINES);
}

enum CarState
{
    ON_MAIN_ROAD,
    ENTERING,
    MOVING_TO_SLOT,
    WAITING,
    MOVING_BETWEEN_SLOTS,
    CHARGING,
    EXITING,
    MERGING_OUT
};

struct ChargingSlot
{
    Rectangle rect;
    int id;
    int occupiedByCarID = -1;
    float freeTimer = Config::MOVE_DELAY;
    Vector2 GetCenter() const { return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f}; }
};

const int PARKING_SCREEN_WIDTH = 1200;
const int PARKING_SCREEN_HEIGHT = 800;
const float DEG_TO_RAD = 3.14159f / 180.0f;

struct RoadMap
{
    float mainY, leftX, rightX, turnTopY, topLimitY, centerX;
    float row1_InnerY, row1_OuterY, row2_InnerY, middle_SharedOuterY, row3_InnerY, row4_InnerY, row4_OuterY;
};

#endif // SHARED_H