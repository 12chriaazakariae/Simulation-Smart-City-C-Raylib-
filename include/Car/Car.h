#ifndef CAR_H
#define CAR_H

#include "../Config/Shared.h"
#include "../Parking/ParkingSpot.h"

// Represents a vehicle operating within the EV Charging Station zone
// Handles charging logic, pathfinding into slots, and merging back to the main road
class ChargingCar
{
public:
    int id;
    Vector2 position;
    float angle;
    CarState state;
    bool isElectric;
    float batteryLevel;
    float budget;
    bool decisionMade = false;
    float chargeTimer;
    deque<Vector2> path;
    int targetSlotIdx = -1;

    // Visual variant ID for texture selection
    int textureVariant;

    // Movement physics properties
    float currentSpeed;
    float acceleration;
    float friction;

    ChargingCar(int _id, Vector2 startPos, bool _isEV, float _batt, float _budget)
        : id(_id), position(startPos), isElectric(_isEV), batteryLevel(_batt), budget(_budget)
    {
        state = ON_MAIN_ROAD;
        angle = 0.0f;
        chargeTimer = 0.0f;

        // Assign a random texture variant (0-3)
        textureVariant = GetRandomValue(0, 3);

        // Initialize physics constants
        currentSpeed = Config::CAR_SPEED_ROAD;
        acceleration = 0.1f;
        friction = 0.2f;
    }

    virtual ~ChargingCar() {}

    // defines the path points to enter the station from the main road
    void GeneratePathIntoStation()
    {
        path.clear();
        path.push_back({Config::RAMP_ENTRY_X, Config::MAIN_ROAD_Y});
        path.push_back({Config::RAMP_ENTRY_X, Config::STATION_BOTTOM_Y});
        path.push_back({Config::RAMP_ENTRY_X + 100.0f, Config::STATION_BOTTOM_Y});
    }

    // defines the path points to merge back onto the main road
    void GeneratePathMergeOut()
    {
        path.clear();
        path.push_back({Config::SCREEN_WIDTH + 200.0f, Config::MAIN_ROAD_Y});
    }

    // Handles state transitions when the car reaches the end of its current path
    void OnPathCompleted()
    {
        if (state == ENTERING)
        {
            angle = 0.0f;
        }
        else if (state == MOVING_TO_SLOT || state == MOVING_BETWEEN_SLOTS)
        {
            if (position.y < Config::WAITING_ROW1_Y - 10)
                state = CHARGING;
            else
                state = WAITING;
            angle = 270.0f;
            currentSpeed = 0.0f;
        }
        else if (state == EXITING)
        {
            state = MERGING_OUT;
            GeneratePathMergeOut();
        }
    }

    // Main update loop: Handles movement, charging simulation, and collision avoidance
    virtual void Update(const vector<ChargingCar> *otherCars = nullptr)
    {
        float dt = GetFrameTime();

        // Check if the car should enter the station based on battery level
        if (state == ON_MAIN_ROAD && !decisionMade)
        {
            if (position.x >= Config::RAMP_ENTRY_X - 10 && position.x <= Config::RAMP_ENTRY_X + 10)
            {
                decisionMade = true;
                if (isElectric && batteryLevel <= 50.0f)
                {
                    state = ENTERING;
                    GeneratePathIntoStation();
                }
                else
                {
                    path.clear();
                    path.push_back({Config::SCREEN_WIDTH + 200.0f, Config::MAIN_ROAD_Y});
                }
            }
        }

        // Simulate battery charging and auto-exit when full or timer expires
        if (state == CHARGING)
        {
            chargeTimer += dt;
            batteryLevel += dt * (100.0f / Config::CHARGING_TIME);
            if (batteryLevel >= 100.0f)
                batteryLevel = 100.0f;

            if (batteryLevel >= 100.0f || chargeTimer >= Config::CHARGING_TIME)
            {
                chargeTimer = Config::CHARGING_TIME;
                state = EXITING;
            }
            return;
        }

        if (state == WAITING)
            return;

        float targetSpeed = (state == ON_MAIN_ROAD || state == MERGING_OUT) ? Config::CAR_SPEED_ROAD : Config::CAR_SPEED_STATION;

        // Logic for cars exiting the station: Checks for traffic on the main road before merging
        if (state == EXITING && otherCars != nullptr)
        {
            bool approachingExit = (position.y > Config::STATION_BOTTOM_Y &&
                                    abs(position.x - Config::EXIT_LANE_X) < 20.0f);

            if (approachingExit)
            {
                bool isFirstInQueue = true;
                float distToCarAhead = 10000.0f;

                // Check if there are other exiting cars ahead in the queue
                for (const auto &other : *otherCars)
                {
                    if (other.id == this->id)
                        continue;

                    if (other.state == EXITING &&
                        abs(other.position.x - position.x) < 20.0f &&
                        other.position.y > position.y)
                    {
                        isFirstInQueue = false;
                        float d = other.position.y - position.y;
                        if (d < distToCarAhead)
                            distToCarAhead = d;
                    }
                }

                bool shouldStop = false;

                // If first in queue, check the main road sensor box for incoming traffic
                if (isFirstInQueue)
                {
                    Rectangle sensorBox = {Config::EXIT_LANE_X - 100.0f, Config::MAIN_ROAD_Y - 35.0f, 200.0f, 70.0f};

                    for (const auto &other : *otherCars)
                    {
                        if (other.id == this->id)
                            continue;

                        if (other.state == ON_MAIN_ROAD || other.state == MERGING_OUT)
                        {
                            Rectangle otherRect = {other.position.x - 20, other.position.y - 30, 40, 60};
                            if (CheckCollisionRecs(sensorBox, otherRect))
                            {
                                shouldStop = true;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // If not first, just maintain distance from the car ahead
                    if (distToCarAhead < 90.0f)
                    {
                        shouldStop = true;
                    }
                }

                if (shouldStop)
                {
                    targetSpeed = 0.0f;
                }
            }
        }

        // Follow the calculated path points
        if (!path.empty())
        {
            Vector2 target = path.front();
            float dx = target.x - position.x;
            float dy = target.y - position.y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist < 10.0f)
            {
                path.pop_front();
                if (path.empty())
                    OnPathCompleted();
                return;
            }

            // Smoothly rotate towards target
            float targetAngle = atan2(dy, dx) * (180.0f / PI);
            float angleDiff = GetAngleDiff(angle, targetAngle);

            if (abs(angleDiff) > 2.0f)
            {
                if (angleDiff > 0)
                    angle += Config::TURN_SPEED;
                else
                    angle -= Config::TURN_SPEED;
            }
            else
            {
                angle = targetAngle;
            }

            if (abs(angleDiff) > 20.0f)
                targetSpeed *= 0.6f;

            // Merge collision check: Ensure we don't hit cars while merging out
            if ((state == MERGING_OUT || state == EXITING) && otherCars != nullptr)
            {
                Rectangle myRect = {position.x - 20, position.y - 30, 40, 60};
                myRect.x += cos(angle * DEG_TO_RAD) * 60.0f;
                myRect.y += sin(angle * DEG_TO_RAD) * 60.0f;

                for (const auto &other : *otherCars)
                {
                    if (other.id == this->id)
                        continue;

                    if (other.state == ON_MAIN_ROAD || other.state == MERGING_OUT)
                    {
                        Rectangle otherRect = {other.position.x - 25, other.position.y - 35, 50, 70};
                        if (CheckCollisionRecs(myRect, otherRect))
                        {
                            targetSpeed = 0.0f;
                            break;
                        }
                    }
                }
            }

            // Apply acceleration or friction
            if (currentSpeed < targetSpeed)
                currentSpeed += acceleration;
            else if (currentSpeed > targetSpeed)
                currentSpeed -= friction;

            if (currentSpeed < 0)
                currentSpeed = 0;

            float rad = angle * (PI / 180.0f);
            position.x += cos(rad) * currentSpeed;
            position.y += sin(rad) * currentSpeed;
        }
        else if (state == ON_MAIN_ROAD && !decisionMade)
        {
            if (currentSpeed < targetSpeed)
                currentSpeed += acceleration;
            else if (currentSpeed > targetSpeed)
                currentSpeed -= friction;

            position.x += currentSpeed;
        }
    }

    // Renders the car using the assigned texture variant and battery indicator
    virtual void Draw(Texture2D carTexture)
    {
        Rectangle sourceRec = {0, 0, (float)carTexture.width, (float)carTexture.height};
        Rectangle destRec = {position.x, position.y, Config::CAR_WIDTH, Config::CAR_HEIGHT};
        Vector2 origin = {Config::CAR_WIDTH / 2.0f, Config::CAR_HEIGHT / 2.0f};

        DrawTexturePro(carTexture, sourceRec, destRec, origin, angle + 90.0f, WHITE);

        Color overlayColor = BLANK;
        DrawRectanglePro(destRec, origin, angle + 90.0f, overlayColor);

        // Draw battery bar if electric
        if (isElectric)
        {
            DrawRectangle(position.x - 20, position.y - 40, 40, 6, BLACK);
            Color batColor = (batteryLevel <= 50.0f) ? RED : GREEN;
            if (state == CHARGING)
                batColor = YELLOW;
            DrawRectangle(position.x - 20, position.y - 40, (int)(40 * (batteryLevel / 100.0f)), 6, batColor);
        }

        DrawText(TextFormat("$%.0f", budget), position.x - 10, position.y - 50, 10, DARKGREEN);
    }
};

// Represents a vehicle operating within the Smart Parking Lot zone
// Handles pathfinding to specific spots, parking maneuvers, and exiting
class ParkingCar
{
protected:
    Vector2 position;
    float angle, speed, width, height;
    Color color;
    std::deque<Vector2> path;
    bool hasTarget;
    float acceleration, turnSpeed, friction;
    bool isActive;
    float parkingTimer;
    bool isParked, isLeaving, isReversing;
    int assignedSpotID;
    float entryAngle;
    bool isBlocked;
    int blockedByIndex;

public:
    int textureVariant;

    ParkingCar(float startX, float startY, float startAngle)
    {
        position = {startX + Config::PARKING_OFFSET_X, startY};
        width = PARKING_SCREEN_WIDTH / 30.0f;
        height = PARKING_SCREEN_HEIGHT / 12.5f;
        angle = startAngle;
        speed = 0.0f;
        hasTarget = false;
        color = DARKGRAY;
        acceleration = PARKING_SCREEN_HEIGHT / 5000.0f;
        turnSpeed = 3.5f;
        friction = 0.96f;
        isActive = false;
        parkingTimer = 0.0f;
        isParked = false;
        isLeaving = false;
        isReversing = false;
        assignedSpotID = -1;
        entryAngle = 0.0f;
        isBlocked = false;
        blockedByIndex = -1;

        textureVariant = GetRandomValue(0, 3);
    }
    void SetColor(Color c) { color = c; }
    void SetSpeed(float s) { speed = s; }

    void SetBlocked(bool blocked, int blockerIdx = -1)
    {
        isBlocked = blocked;
        blockedByIndex = blockerIdx;
    }
    void ForceUnblock()
    {
        isBlocked = false;
        blockedByIndex = -1;
    }
    Vector2 GetPosition() const { return position; }
    bool IsLeaving() const { return isLeaving; }
    bool IsParked() const { return isParked; }
    bool IsActive() const { return isActive; }
    void Activate() { isActive = true; }
    float GetParkingTimer() const { return parkingTimer; }
    int GetAssignedSpotID() const { return assignedSpotID; }
    int GetBlockedByIndex() const { return blockedByIndex; }
    bool HasPath() const { return !path.empty(); }
    void SetPosition(Vector2 pos) { position = {pos.x + Config::PARKING_OFFSET_X, pos.y}; }
    void SetPositionDirect(Vector2 pos) { position = pos; }
    void SetAngle(float a)
    {
        angle = a;
        entryAngle = a;
    }
    void SetAsParked(int spotID, float initialTimer = 0.0f)
    {
        assignedSpotID = spotID;
        isParked = true;
        isActive = true;
        parkingTimer = initialTimer;
    }
    Rectangle GetSafetyRect() const
    {
        float margin = 5.0f;
        float size = (width > height ? width : height) + (margin * 2);
        return {position.x - size / 2.0f, position.y - size / 2.0f, size, size};
    }

    // Checks if the path is blocked by another car
    bool IsPathBlockedBy(const ParkingCar &other) const
    {
        float dx = other.GetPosition().x - position.x;
        float dy = other.GetPosition().y - position.y;
        float rads = (angle - 90.0f) * DEG_TO_RAD;
        if (isReversing)
            rads += 3.14159f;
        float dirX = cos(rads), dirY = sin(rads);
        float rightX = -dirY, rightY = dirX;
        float forwardDist = (dx * dirX) + (dy * dirY);
        float sideDist = abs((dx * rightX) + (dy * rightY));
        if (forwardDist <= 0)
            return false;
        float safetyGap = 20.0f;
        if (forwardDist > (height / 2.0f + other.height / 2.0f + safetyGap))
            return false;
        float laneWidthThreshold = (width + other.width) / 2.0f * 0.7f;
        if (sideDist > laneWidthThreshold)
            return false;
        return true;
    }

    // Calculates the path to a specific parking spot based on its ID and road layout
    void SetTargetSpot(const ParkingSpot &spot, const RoadMap &roads)
    {
        path.clear();
        hasTarget = true;
        assignedSpotID = spot.GetID();
        isReversing = false;
        Vector2 spotCenter = spot.GetCenter();
        int spotID = spot.GetID();
        bool isLeftSide = (spotID >= 1 && spotID <= 20);
        float targetLaneY = 0;
        if ((spotID >= 1 && spotID <= 5) || (spotID >= 21 && spotID <= 25))
            targetLaneY = isLeftSide ? roads.row1_InnerY : roads.row1_OuterY;
        else if ((spotID >= 16 && spotID <= 20) || (spotID >= 36 && spotID <= 40))
            targetLaneY = isLeftSide ? roads.row4_OuterY : roads.row4_InnerY;
        else
        {
            bool isRow2 = ((spotID >= 6 && spotID <= 10) || (spotID >= 26 && spotID <= 30));
            if (isLeftSide)
                targetLaneY = isRow2 ? roads.row2_InnerY : roads.row3_InnerY;
            else
                targetLaneY = roads.middle_SharedOuterY;
        }
        if (isLeftSide)
        {
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, roads.mainY});
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, targetLaneY});
            path.push_back({spotCenter.x, targetLaneY});
            path.push_back(spotCenter);
        }
        else
        {
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, roads.mainY});
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, roads.turnTopY});
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, roads.turnTopY});
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, targetLaneY});
            path.push_back({spotCenter.x, targetLaneY});
            path.push_back(spotCenter);
        }
    }

    // Calculates path to exit the parking lot from the current spot
    void StartLeaving(const ParkingSpot &spot, const RoadMap &roads)
    {
        isLeaving = true;
        isParked = false;
        isReversing = true;
        path.clear();
        Vector2 spotCenter = spot.GetCenter();
        int spotID = spot.GetID();
        bool isLeftSide = (spotID >= 1 && spotID <= 20);
        float reverseToY = 0;
        if ((spotID >= 1 && spotID <= 5) || (spotID >= 21 && spotID <= 25))
            reverseToY = isLeftSide ? roads.row1_OuterY : roads.row1_InnerY;
        else if ((spotID >= 16 && spotID <= 20) || (spotID >= 36 && spotID <= 40))
            reverseToY = isLeftSide ? roads.row4_InnerY : roads.row4_OuterY;
        else
        {
            bool isRow2 = ((spotID >= 6 && spotID <= 10) || (spotID >= 26 && spotID <= 30));
            if (isLeftSide)
                reverseToY = roads.middle_SharedOuterY;
            else
                reverseToY = isRow2 ? roads.row2_InnerY : roads.row3_InnerY;
        }
        path.push_back({spotCenter.x, reverseToY});
        if (isLeftSide)
        {
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, reverseToY});
            path.push_back({roads.leftX + Config::PARKING_OFFSET_X, roads.turnTopY});
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, roads.turnTopY});
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, roads.mainY});
            path.push_back({Config::SCREEN_WIDTH + 600, roads.mainY});
        }
        else
        {
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, reverseToY});
            path.push_back({roads.rightX + Config::PARKING_OFFSET_X, roads.mainY});
            path.push_back({Config::SCREEN_WIDTH + 600, roads.mainY});
        }
        hasTarget = true;
    }

    // Updates physics, timers, and calls AI logic
    void Update()
    {
        if (!isActive)
            return;
        if (path.empty() && hasTarget)
        {
            speed = 0.0f;
            hasTarget = false;
            isReversing = false;
        }
        if (!isLeaving && !hasTarget && isActive && !isParked)
        {
            isParked = true;
            parkingTimer = 0.0f;
        }
        if (isParked)
            parkingTimer += GetFrameTime();
        if (hasTarget)
            HandleAI();
        speed *= friction;
        if (isBlocked && !isParked)
            speed = 0.0f;
        float rads = (angle - 90.0f) * DEG_TO_RAD;
        if (isReversing)
        {
            position.x -= cos(rads) * speed;
            position.y -= sin(rads) * speed;
        }
        else
        {
            position.x += cos(rads) * speed;
            position.y += sin(rads) * speed;
        }
    }

    // Calculates steering and acceleration to follow the path
    void HandleAI()
    {
        if (isBlocked && speed < 0.1f)
            return;
        if (path.empty())
        {
            speed = 0;
            hasTarget = false;
            isReversing = false;
            if (!isLeaving && !isParked)
            {
                isParked = true;
                parkingTimer = 0.0f;
                entryAngle = angle;
            }
            return;
        }
        Vector2 target = path.front();
        float dx = target.x - position.x, dy = target.y - position.y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < 15.0f)
        {
            path.pop_front();
            if (isReversing && !path.empty())
                isReversing = false;
            if (path.empty())
            {
                speed = 0;
                hasTarget = false;
                isReversing = false;
                if (!isLeaving && !isParked)
                {
                    isParked = true;
                    parkingTimer = 0.0f;
                    entryAngle = angle;
                }
                return;
            }
            target = path.front();
            dx = target.x - position.x;
            dy = target.y - position.y;
        }
        float targetAngle = atan2(dy, dx) * (180.0f / 3.14159f) + 90.0f;
        if (isReversing)
            targetAngle += 180.0f;
        float angleDiff = targetAngle - angle;
        while (angleDiff > 180)
            angleDiff -= 360;
        while (angleDiff < -180)
            angleDiff += 360;
        if (!isReversing)
        {
            if (angleDiff > turnSpeed)
                angle += turnSpeed;
            else if (angleDiff < -turnSpeed)
                angle -= turnSpeed;
            else
                angle = targetAngle;
        }
        else
        {
            float reverseTurn = turnSpeed * 0.5f;
            if (abs(angleDiff) > 1.0f)
            {
                if (angleDiff > 0)
                    angle += reverseTurn;
                else
                    angle -= reverseTurn;
            }
        }
        float targetSpeed = isReversing ? 1.5f : 3.0f;
        if (abs(angleDiff) < 45.0f || isReversing)
        {
            if (!isBlocked)
            {
                if (speed < targetSpeed)
                    speed += acceleration * (isReversing ? 0.5f : 1.0f);
            }
        }
        else
        {
            speed *= 0.90f;
        }
    }

    void Draw(Texture2D sharedTexture)
    {
        if (!isActive)
            return;
        Rectangle destRec = {position.x, position.y, width, height};
        Vector2 origin = {width / 2.0f, height / 2.0f};

        if (sharedTexture.id > 0)
            DrawTexturePro(sharedTexture, {0, 0, (float)sharedTexture.width, (float)sharedTexture.height}, destRec, origin, angle, WHITE);
        else
        {
            Rectangle outlineRec = {position.x, position.y, width + 2, height + 2};
            DrawRectanglePro(outlineRec, origin, angle, BLACK);
            DrawRectanglePro(destRec, origin, angle, color);
            float rads = (angle - 90.0f) * DEG_TO_RAD;
            Vector2 lightPos = isReversing ? Vector2{position.x - cos(rads) * height * 0.4f, position.y - sin(rads) * height * 0.4f} : Vector2{position.x + cos(rads) * height * 0.4f, position.y + sin(rads) * height * 0.4f};
            DrawCircleV(lightPos, width / 15.0f, isReversing ? RED : YELLOW);
        }
    }
};

#endif // CAR_H