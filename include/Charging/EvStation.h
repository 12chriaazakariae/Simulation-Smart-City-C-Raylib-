#ifndef EV_STATION_H
#define EV_STATION_H

#include "../Config/Shared.h"
#include "../Car/Car.h"
#include "../Car/SmartCar.h"

// Manages the EV charging station, including slot assignment, queuing logic, and rendering
class EVStation
{
private:
    vector<ChargingSlot> chargingSlots;
    vector<ChargingSlot> waitingRow1;
    vector<ChargingSlot> waitingRow2;
    int nextCarID = 0;
    float spawnTimer = 0.0f;

    // Calculates the path for a car to reach a specific charging or waiting slot
    void GeneratePathToSlot(ChargingCar &car, const ChargingSlot &targetSlot)
    {
        car.path.clear();
        Vector2 slotCenter = targetSlot.GetCenter();
        if (car.position.x < slotCenter.x)
            car.path.push_back({slotCenter.x, Config::STATION_BOTTOM_Y});
        car.path.push_back({slotCenter.x, slotCenter.y});
    }

    // Calculates the path for a car to leave the station and merge onto the main road
    void GenerateExitPathStation(ChargingCar &car)
    {
        car.path.clear();
        Vector2 currentPos = car.position;
        car.path.push_back({currentPos.x, Config::TOP_LANE_Y});
        car.path.push_back({Config::EXIT_LANE_X, Config::TOP_LANE_Y});
        car.path.push_back({Config::EXIT_LANE_X, Config::MAIN_ROAD_Y - 60.0f});
        car.path.push_back({Config::EXIT_LANE_X + 60.0f, Config::MAIN_ROAD_Y});
    }

    // Moves cars forward from Waiting Row 2 to Row 1, and from Row 1 to Charging Slots when available
    void ProcessQueue()
    {
        for (int i = 0; i < Config::NUM_SLOTS; i++)
        {
            // Move from Waiting Row 1 to Charging Slot
            if (chargingSlots[i].occupiedByCarID == -1 && chargingSlots[i].freeTimer >= Config::MOVE_DELAY && waitingRow1[i].occupiedByCarID != -1)
            {
                int carID = waitingRow1[i].occupiedByCarID;
                auto it = find_if(cars.begin(), cars.end(), [carID](const ChargingCar &c)
                                  { return c.id == carID; });
                if (it != cars.end())
                {
                    waitingRow1[i].occupiedByCarID = -1;
                    waitingRow1[i].freeTimer = 0.0f;
                    chargingSlots[i].occupiedByCarID = carID;
                    it->state = MOVING_BETWEEN_SLOTS;
                    it->targetSlotIdx = i;
                    it->path.clear();
                    it->path.push_back(chargingSlots[i].GetCenter());
                }
            }
            // Move from Waiting Row 2 to Waiting Row 1
            if (waitingRow1[i].occupiedByCarID == -1 && waitingRow1[i].freeTimer >= Config::MOVE_DELAY && waitingRow2[i].occupiedByCarID != -1)
            {
                int carID = waitingRow2[i].occupiedByCarID;
                auto it = find_if(cars.begin(), cars.end(), [carID](const ChargingCar &c)
                                  { return c.id == carID; });
                if (it != cars.end())
                {
                    waitingRow2[i].occupiedByCarID = -1;
                    waitingRow2[i].freeTimer = 0.0f;
                    waitingRow1[i].occupiedByCarID = carID;
                    it->state = MOVING_BETWEEN_SLOTS;
                    it->targetSlotIdx = i;
                    it->path.clear();
                    it->path.push_back(waitingRow1[i].GetCenter());
                }
            }
        }
    }

public:
    vector<ChargingCar> cars;

    // Initializes the layout of charging slots and waiting rows
    EVStation()
    {
        for (int i = 0; i < Config::NUM_SLOTS; i++)
        {
            float x = Config::FIRST_SLOT_X + i * (Config::SLOT_WIDTH + Config::SLOT_GAP_X);
            chargingSlots.push_back({{x, Config::CHARGING_SLOTS_Y, Config::SLOT_WIDTH, Config::SLOT_HEIGHT}, i + 1, -1});
            waitingRow1.push_back({{x, Config::WAITING_ROW1_Y, Config::SLOT_WIDTH, Config::SLOT_HEIGHT}, i + 1, -1});
            waitingRow2.push_back({{x, Config::WAITING_ROW2_Y, Config::SLOT_WIDTH, Config::SLOT_HEIGHT}, i + 1, -1});
        }
    }

    // Main logic loop: spawns cars, manages queue movement, assigns slots to new arrivals, handles departures, and updates car physics
    void Update()
    {
        float dt = GetFrameTime();

        // Update timers for free slots
        for (auto &s : chargingSlots)
            if (s.occupiedByCarID == -1)
                s.freeTimer += dt;
        for (auto &s : waitingRow1)
            if (s.occupiedByCarID == -1)
                s.freeTimer += dt;
        for (auto &s : waitingRow2)
            if (s.occupiedByCarID == -1)
                s.freeTimer += dt;

        // Spawn new cars randomly
        spawnTimer += dt;
        if (spawnTimer > Config::SPAWN_RATE)
        {
            bool isEV = (GetRandomValue(0, 100) < 60);
            float startBatt = (float)GetRandomValue(10, 100);
            float startBudget = (float)GetRandomValue(10, 60);

            if (isEV)
                cars.push_back(SmartChargingCar(nextCarID++, Vector2{Config::SPAWN_X, Config::MAIN_ROAD_Y}, startBatt, startBudget));
            else
                cars.push_back(ChargingCar(nextCarID++, Vector2{Config::SPAWN_X, Config::MAIN_ROAD_Y}, false, 0.0f, startBudget));
            spawnTimer = 0.0f;
        }

        ProcessQueue();

        for (auto &car : cars)
        {
            // Assign incoming cars to the best available slot (Charging -> Row 1 -> Row 2)
            if (car.state == ENTERING && car.path.empty())
            {
                bool assigned = false;
                for (int i = 0; i < Config::NUM_SLOTS; i++)
                {
                    if (chargingSlots[i].occupiedByCarID == -1 && chargingSlots[i].freeTimer >= Config::MOVE_DELAY && waitingRow1[i].occupiedByCarID == -1 && waitingRow2[i].occupiedByCarID == -1)
                    {
                        chargingSlots[i].occupiedByCarID = car.id;
                        car.targetSlotIdx = i;
                        car.state = MOVING_TO_SLOT;
                        GeneratePathToSlot(car, chargingSlots[i]);
                        assigned = true;
                        break;
                    }
                }
                if (!assigned)
                {
                    for (int i = 0; i < Config::NUM_SLOTS; i++)
                    {
                        if (waitingRow1[i].occupiedByCarID == -1 && waitingRow1[i].freeTimer >= Config::MOVE_DELAY && waitingRow2[i].occupiedByCarID == -1)
                        {
                            waitingRow1[i].occupiedByCarID = car.id;
                            car.targetSlotIdx = i;
                            car.state = MOVING_TO_SLOT;
                            GeneratePathToSlot(car, waitingRow1[i]);
                            assigned = true;
                            break;
                        }
                    }
                }
                if (!assigned)
                {
                    for (int i = 0; i < Config::NUM_SLOTS; i++)
                    {
                        if (waitingRow2[i].occupiedByCarID == -1 && waitingRow2[i].freeTimer >= Config::MOVE_DELAY)
                        {
                            waitingRow2[i].occupiedByCarID = car.id;
                            car.targetSlotIdx = i;
                            car.state = MOVING_TO_SLOT;
                            GeneratePathToSlot(car, waitingRow2[i]);
                            assigned = true;
                            break;
                        }
                    }
                }
                if (!assigned && car.position.x < Config::SCREEN_WIDTH - 100)
                    car.position.x += 0.5f;
            }

            // Handle car departure
            if (car.state == EXITING && car.path.empty())
            {
                if (car.targetSlotIdx != -1)
                {
                    chargingSlots[car.targetSlotIdx].occupiedByCarID = -1;
                    chargingSlots[car.targetSlotIdx].freeTimer = 0.0f;
                    car.targetSlotIdx = -1;
                }
                GenerateExitPathStation(car);
            }

            // Update individual car physics and collision checks
            car.Update(&cars);
        }
    }

    // Renders the station background, road markings, slots, and all cars using their assigned textures
    void Draw(const std::vector<Texture2D> &textureList)
    {
        // Draw Foundation (Curb and Asphalt)
        Rectangle stationBase = {40, 20, 1130, 750};
        DrawRectangleRec(stationBase, Colors::CURB);
        DrawRectangleRec({stationBase.x + 10, stationBase.y + 10, stationBase.width - 20, stationBase.height - 20}, Colors::ASPHALT);

        float entryX = Config::RAMP_ENTRY_X;
        float exitX = Config::EXIT_LANE_X;
        float bottomY = Config::STATION_BOTTOM_Y;
        float mainY = Config::MAIN_ROAD_Y;

        // Draw Ramps
        DrawRectangle(entryX - 40, bottomY, 80, (mainY - bottomY) + 5, Colors::ASPHALT);
        DrawRectangle(exitX - 40, bottomY, 80, (mainY - bottomY) + 5, Colors::ASPHALT);

        // Draw Fluid Curves and Borders
        DrawLineEx({entryX - 40, bottomY}, {entryX - 40, mainY - 60}, 3, WHITE);
        DrawLineEx({entryX + 40, bottomY}, {entryX + 40, mainY - 60}, 3, WHITE);
        DrawRoadCurve(entryX - 40, mainY - 60, 20, 3);
        DrawRoadCurve(entryX + 40, mainY - 60, 20, 2);

        DrawLineEx({exitX - 40, bottomY}, {exitX - 40, mainY - 60}, 3, WHITE);
        DrawLineEx({exitX + 40, bottomY}, {exitX + 40, mainY - 60}, 3, WHITE);
        DrawRoadCurve(exitX - 40, mainY - 60, 20, 3);
        DrawRoadCurve(exitX + 40, mainY - 60, 20, 2);

        // Draw Slots and Waiting Rows
        auto DrawUnit = [](const ChargingSlot &s)
        {
            DrawRectangleRec(s.rect, Colors::PARKING_BG);
            DrawRectangleLinesEx(s.rect, 2, Colors::LINES);
            DrawRectangle(s.rect.x + s.rect.width / 2 - 10, s.rect.y - 15, 20, 15, DARKGRAY);
            DrawCircle(s.rect.x + s.rect.width / 2, s.rect.y - 15, 5, GREEN);
            if (s.occupiedByCarID == -1)
                DrawText("FREE", s.rect.x + 15, s.rect.y + 40, 10, GREEN);
        };

        for (const auto &slot : chargingSlots)
            DrawUnit(slot);
        for (const auto &slot : waitingRow1)
            DrawRectangleLinesEx(slot.rect, 5, Fade(WHITE, 0.3f));
        for (const auto &slot : waitingRow2)
            DrawRectangleLinesEx(slot.rect, 5, Fade(WHITE, 0.3f));

        DrawText("EV ZONE", 60, 40, 30, LIGHTGRAY);

        // Draw all cars using their assigned random texture variant
        for (auto &car : cars)
        {
            if (car.textureVariant >= 0 && car.textureVariant < textureList.size())
                car.Draw(textureList[car.textureVariant]);
            else
                car.Draw(textureList[0]);
        }
    }
};

#endif // EV_STATION_H