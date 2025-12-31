// ============================================================================
// SmartCity.CPP - Smart Parking System Main Application
// ============================================================================

#include "../include/Config/Shared.h"
#include "../include/Car/Car.h"
#include "../include/Car/SmartCar.h"
#include "../include/Parking/ParkingSpot.h"
#include "../include/Charging/EvStation.h"
#include "../include/Parking/ParkingManager.h"

// Helper to draw dashed lines
void DrawDashedLine(Vector2 start, Vector2 end, float thick, float dashLen, float gapLen, Color color)
{
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float dist = sqrt(dx * dx + dy * dy);
    float angle = atan2(dy, dx);
    int numDashes = dist / (dashLen + gapLen);

    for (int i = 0; i < numDashes; i++)
    {
        float currentDist = i * (dashLen + gapLen);
        Vector2 p1 = {start.x + cos(angle) * currentDist, start.y + sin(angle) * currentDist};
        Vector2 p2 = {start.x + cos(angle) * (currentDist + dashLen), start.y + sin(angle) * (currentDist + dashLen)};
        DrawLineEx(p1, p2, thick, color);
    }
}

// 1. ADD STATE ENUM
enum AppState
{
    MENU,
    GAME
};

int main()
{
    InitWindow(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "Smart Parking System");
    SetTargetFPS(60);
    ToggleFullscreen();

    // ========================================================================================
    // MENU SETUP
    // ========================================================================================
    AppState currentState = MENU;

    // Load your specific background image
    Texture2D menuBg = LoadTexture("assets/SmartParking.jpeg");
    bool hasBg = (menuBg.id > 0);

    // Button Setup
    float btnWidth = 320.0f;
    float btnHeight = 60.0f;
    Rectangle startBtnBounds, exitBtnBounds;

    // ========================================================================================
    // LOAD CAR TEXTURES (The 4 Random Variants)
    // ========================================================================================
    std::vector<Texture2D> carTextures;
    carTextures.push_back(LoadTexture("assets/cars/car.png"));  // ID 0
    carTextures.push_back(LoadTexture("assets/cars/car3.png")); // ID 1
    carTextures.push_back(LoadTexture("assets/cars/car4.png")); // ID 2
    carTextures.push_back(LoadTexture("assets/cars/cars.png")); // ID 3

    // Fallback check
    if (carTextures[0].id == 0)
        TraceLog(LOG_WARNING, "Failed to load cars/car.png");

    // ========================================================================================
    // SIMULATION SETUP (YOUR ORIGINAL LOGIC)
    // ========================================================================================
    EVStation chargingStation;

    float spotW = PARKING_SCREEN_WIDTH / 18.75f;
    float spotH = PARKING_SCREEN_HEIGHT / 10.0f;
    float gap = PARKING_SCREEN_WIDTH / 37.5f;
    float margin = PARKING_SCREEN_WIDTH / 100.0f;
    float laneOffset = 80.0f;
    float distFromSpot = 40.0f;
    float mainRoadY = Config::MAIN_ROAD_Y;
    float roadTopEdgeY = mainRoadY - 30.0f;
    float safetyBuffer = 25.0f;

    float row4_OuterY_Pos = roadTopEdgeY - safetyBuffer;
    float row4_InnerY_Pos = row4_OuterY_Pos - laneOffset;
    float startY_Bottom = row4_InnerY_Pos - distFromSpot - spotH;
    float startY_Top = startY_Bottom - spotH;
    float row3_InnerY_Pos = startY_Top - distFromSpot;
    float sharedMiddleY_Pos = row3_InnerY_Pos - laneOffset;
    float row2_InnerY_Pos = sharedMiddleY_Pos - laneOffset;
    float startY_Upper_Bottom = row2_InnerY_Pos - distFromSpot - spotH;
    float startY_Upper_Top = startY_Upper_Bottom - spotH;
    float row1_BaseY = startY_Upper_Top - distFromSpot;

    std::vector<ParkingSpot> parkingSpots;
    int spotsPerRow = 5;
    float startX_Left = margin * 3.0f;
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 1, startX_Left + (spotW + gap) * i, startY_Upper_Top, spotW, spotH, false));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 6, startX_Left + (spotW + gap) * i, startY_Upper_Bottom, spotW, spotH, true));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 11, startX_Left + (spotW + gap) * i, startY_Top, spotW, spotH, false));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 16, startX_Left + (spotW + gap) * i, startY_Bottom, spotW, spotH, true));
    float startX_Right = PARKING_SCREEN_WIDTH - ((spotW * spotsPerRow) + (gap * (spotsPerRow - 1))) - (margin * 3.0f);
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 21, startX_Right + (spotW + gap) * i, startY_Upper_Top, spotW, spotH, false));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 26, startX_Right + (spotW + gap) * i, startY_Upper_Bottom, spotW, spotH, true));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 31, startX_Right + (spotW + gap) * i, startY_Top, spotW, spotH, false));
    for (int i = 0; i < spotsPerRow; i++)
        parkingSpots.push_back(ParkingSpot(i + 36, startX_Right + (spotW + gap) * i, startY_Bottom, spotW, spotH, true));

    RoadMap parkingRoads;
    parkingRoads.mainY = mainRoadY;
    parkingRoads.leftX = (PARKING_SCREEN_WIDTH / 2.0f) - 50.0f;
    parkingRoads.rightX = (PARKING_SCREEN_WIDTH / 2.0f) + 50.0f;
    parkingRoads.centerX = PARKING_SCREEN_WIDTH / 2.0f;
    parkingRoads.turnTopY = startY_Upper_Top - 150.0f;
    parkingRoads.topLimitY = parkingRoads.turnTopY;
    parkingRoads.row4_InnerY = row4_InnerY_Pos;
    parkingRoads.row4_OuterY = row4_OuterY_Pos;
    parkingRoads.row3_InnerY = row3_InnerY_Pos;
    parkingRoads.middle_SharedOuterY = sharedMiddleY_Pos;
    parkingRoads.row2_InnerY = row2_InnerY_Pos;
    parkingRoads.row1_InnerY = row1_BaseY;
    parkingRoads.row1_OuterY = row1_BaseY - laneOffset;

    vector<int> chargers = {};
    ParkingManager parkingManager(&parkingSpots, chargers);
    std::vector<SmartParkingCar> parkingCars;

    Camera2D camera = {0};
    camera.zoom = 0.55f;
    camera.target = {Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f};
    camera.offset = {Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f};

    while (!WindowShouldClose())
    {
        // ----------------------------------------------------------------------------------
        // UPDATE PHASE
        // ----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();
        float scrW = (float)GetScreenWidth();
        float scrH = (float)GetScreenHeight();

        if (currentState == MENU)
        {
            float startY = scrH * 0.65f;
            startBtnBounds = {(scrW / 2 - 220) - btnWidth / 2, startY, btnWidth, btnHeight};
            exitBtnBounds = {(scrW / 2 - 220) - btnWidth / 2, startY + btnHeight + 20, btnWidth, btnHeight};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mousePos, startBtnBounds))
                    currentState = GAME;
                if (CheckCollisionPointRec(mousePos, exitBtnBounds))
                    break;
            }
        }
        else // currentState == GAME
        {
            float dt = GetFrameTime();
            if (IsKeyDown(KEY_RIGHT))
                camera.target.x += 500.0f * dt;
            if (IsKeyDown(KEY_LEFT))
                camera.target.x -= 500.0f * dt;
            if (IsKeyDown(KEY_DOWN))
                camera.target.y += 500.0f * dt;
            if (IsKeyDown(KEY_UP))
                camera.target.y -= 500.0f * dt;
            float wheel = GetMouseWheelMove();
            if (wheel != 0)
            {
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.zoom += wheel * 0.1f;
                if (camera.zoom < 0.2f)
                    camera.zoom = 0.2f;
                Vector2 mouseWorldPosAfter = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.target.x += (mouseWorldPos.x - mouseWorldPosAfter.x);
                camera.target.y += (mouseWorldPos.y - mouseWorldPosAfter.y);
            }
            if (IsKeyPressed(KEY_R))
            {
                camera.target = {Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f};
                camera.zoom = 0.55f;
            }

            // Update Logic
            chargingStation.Update();

            // --------------------------------------------------------------------
            // HANDOFF: STATION -> PARKING (PRESERVE TEXTURE ID)
            // --------------------------------------------------------------------
            auto &cList = chargingStation.cars;
            for (auto it = cList.begin(); it != cList.end();)
            {
                if (it->position.x > Config::PARKING_OFFSET_X - 20.0f)
                {
                    Vector2 pos = it->position;
                    bool isEV = it->isElectric;
                    float batt = it->batteryLevel;
                    float budget = it->budget;
                    int textureID = it->textureVariant; // <--- GRAB ID FROM STATION CAR

                    SmartParkingCar newCar(pos.x - Config::PARKING_OFFSET_X, pos.y, isEV, budget);
                    newCar.batteryLevel = batt;
                    newCar.textureVariant = textureID; // <--- GIVE ID TO PARKING CAR

                    if (newCar.isElectric)
                        newCar.needsCharging = (newCar.batteryLevel < 30.0f);
                    newCar.SetPositionDirect(pos);
                    newCar.SetAngle(90.0f);
                    newCar.SetSpeed(Config::CAR_SPEED_ROAD);
                    newCar.Activate();
                    parkingCars.push_back(newCar);
                    it = cList.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // Parking System Updates
            for (auto &car : parkingCars)
                if (!car.IsActive() && !car.IsParked() && !car.IsLeaving())
                    car.Activate();
            for (auto &car : parkingCars)
                car.SetBlocked(false);
            for (int i = 0; i < parkingCars.size(); i++)
            {
                if (!parkingCars[i].IsActive() || parkingCars[i].IsParked())
                    continue;
                for (int j = 0; j < parkingCars.size(); j++)
                {
                    if (i == j)
                        continue;
                    if (!parkingCars[j].IsActive())
                        continue;
                    if (parkingCars[i].IsPathBlockedBy(parkingCars[j]))
                        parkingCars[i].SetBlocked(true, j);
                }
            }
            for (int i = 0; i < parkingCars.size(); i++)
            {
                if (parkingCars[i].IsActive() && !parkingCars[i].IsParked())
                {
                    int blockerIdx = parkingCars[i].GetBlockedByIndex();
                    if (blockerIdx != -1 && parkingCars[blockerIdx].GetBlockedByIndex() == i)
                    {
                        if (i < blockerIdx)
                            parkingCars[i].ForceUnblock();
                        else
                            parkingCars[blockerIdx].ForceUnblock();
                    }
                }
            }

            for (int i = 0; i < parkingCars.size(); i++)
            {
                if (!parkingCars[i].IsActive())
                    continue;
                if (parkingCars[i].GetPosition().x > Config::SCREEN_WIDTH + 100)
                {
                    parkingCars.erase(parkingCars.begin() + i);
                    i--;
                    continue;
                }
                if (parkingCars[i].GetAssignedSpotID() == -1 && !parkingCars[i].IsParked() && !parkingCars[i].IsLeaving())
                {
                    ParkingSpot *freeSpot = parkingManager.RequestBestSpot(parkingCars[i].GetPosition(), parkingCars[i].isElectric, parkingCars[i].needsCharging, parkingCars[i].budget, parkingCars);
                    if (freeSpot != nullptr)
                    {
                        freeSpot->SetReserved(true);
                        parkingCars[i].SetTargetSpot(*freeSpot, parkingRoads);
                    }
                }
                if (parkingCars[i].IsParked() && parkingCars[i].GetParkingTimer() >= 50.0f && !parkingCars[i].IsLeaving())
                {
                    ParkingSpot *spot = parkingManager.GetSpotByID(parkingCars[i].GetAssignedSpotID());
                    if (spot != nullptr)
                        parkingCars[i].StartLeaving(*spot, parkingRoads);
                }
                parkingCars[i].UpdateSmart(parkingManager.GetPrice(parkingCars[i].GetAssignedSpotID()));
            }

            for (auto &spot : parkingSpots)
            {
                bool isNowOccupied = false;
                bool shouldStayReserved = false;
                for (auto &car : parkingCars)
                {
                    if (car.IsActive() && car.GetAssignedSpotID() == spot.GetID())
                    {
                        if (CheckCollisionPointRec(car.GetPosition(), spot.GetRect()))
                        {
                            if (!car.IsLeaving())
                            {
                                isNowOccupied = true;
                                shouldStayReserved = true;
                            }
                        }
                        else
                        {
                            if (!car.IsLeaving())
                                shouldStayReserved = true;
                        }
                    }
                }
                spot.SetOccupied(isNowOccupied);
                spot.SetReserved(shouldStayReserved);
            }
        }

        // ----------------------------------------------------------------------------------
        // DRAWING PHASE
        // ----------------------------------------------------------------------------------
        BeginDrawing();

        if (currentState == MENU)
        {
            // --- DRAW MENU ---
            ClearBackground(BLACK);
            float scale = 1.5f;
            if (hasBg)
            {
                DrawTexturePro(
                    menuBg,
                    Rectangle{0, 0, (float)menuBg.width, (float)menuBg.height},
                    Rectangle{0, 0, menuBg.width * scale, menuBg.height * scale},
                    Vector2{0, 0}, 0.0f, WHITE);
            }
            else
            {
                DrawRectangleGradientV(0, 0, scrW, scrH, DARKBLUE, BLACK);
                const char *t = "SMART PARKING";
                DrawText(t, scrW / 2 - MeasureText(t, 100) / 2, scrH / 2 - 200, 100, SKYBLUE);
            }

            // Buttons
            bool hoverStart = CheckCollisionPointRec(mousePos, startBtnBounds);
            Color fillStart = hoverStart ? Fade(SKYBLUE, 0.5f) : Fade(BLACK, 0.7f);
            Color bordStart = hoverStart ? SKYBLUE : WHITE;

            DrawRectangleRec(startBtnBounds, fillStart);
            DrawRectangleLinesEx(startBtnBounds, 2, bordStart);
            const char *txt1 = "START";
            int fs = 30;
            DrawText(txt1, startBtnBounds.x + (btnWidth - MeasureText(txt1, fs)) / 2,
                     startBtnBounds.y + (btnHeight - fs) / 2, fs, bordStart);

            bool hoverExit = CheckCollisionPointRec(mousePos, exitBtnBounds);
            Color fillExit = hoverExit ? Fade(RED, 0.5f) : Fade(BLACK, 0.7f);
            Color bordExit = hoverExit ? RED : WHITE;

            DrawRectangleRec(exitBtnBounds, fillExit);
            DrawRectangleLinesEx(exitBtnBounds, 2, bordExit);
            const char *txt2 = "EXIT";
            DrawText(txt2, exitBtnBounds.x + (btnWidth - MeasureText(txt2, fs)) / 2,
                     exitBtnBounds.y + (btnHeight - fs) / 2, fs, bordExit);

            DrawCircleV(mousePos, 5, bordStart);
        }
        else // DRAW GAME
        {
            // --- DRAW SIMULATION ---
            ClearBackground(Colors::GRASS);
            BeginMode2D(camera);

            // LAYER 1: FOUNDATION (Curb)
            float pOffset = Config::PARKING_OFFSET_X;
            Rectangle parkingArea = {pOffset + 20, parkingRoads.topLimitY - 20, 1160, (Config::MAIN_ROAD_Y - parkingRoads.topLimitY) + 20};
            DrawRectangleRec(parkingArea, Colors::CURB);

            // MODIFIED: Start Main Road Curb from -300 instead of 0
            DrawRectangle(-300, Config::MAIN_ROAD_Y - 70, Config::SCREEN_WIDTH * 2 + 300, 140, Colors::CURB);

            // LAYER 2: ASPHALT
            // MODIFIED: Start Main Road Asphalt from -300
            DrawRectangle(-300, Config::MAIN_ROAD_Y - 60, Config::SCREEN_WIDTH * 2 + 300, 120, Colors::ASPHALT);

            // Parking Roads (Vertical)
            float vRoadW = 100.0f;
            DrawRectangle(pOffset + parkingRoads.leftX - vRoadW / 2, parkingRoads.topLimitY - 20, vRoadW, parkingArea.height, Colors::ASPHALT);
            DrawRectangle(pOffset + parkingRoads.rightX - vRoadW / 2, parkingRoads.topLimitY - 20, vRoadW, parkingArea.height, Colors::ASPHALT);

            // Parking Roads (Horizontal)
            float hRoadH = 60.0f;
            DrawRectangle(pOffset + 20, parkingRoads.topLimitY, 1160, hRoadH + 20, Colors::ASPHALT);                   // Top
            DrawRectangle(pOffset + 20, parkingRoads.middle_SharedOuterY - hRoadH / 2, 1160, hRoadH, Colors::ASPHALT); // Mid
            DrawRectangle(pOffset + 20, parkingRoads.row4_OuterY - 20, 1160, hRoadH, Colors::ASPHALT);                 // Bot

            // LAYER 3: FLUID BORDERS & MARKINGS
            float thick = 3.0f;
            Color w = Colors::LINES;

            // A. Main Road Dashed Line
            // MODIFIED: Start Dashed Line from -300
            DrawDashedLine({-300, Config::MAIN_ROAD_Y}, {Config::SCREEN_WIDTH * 1.5f, Config::MAIN_ROAD_Y}, 3, 30, 30, w);

            // B. Parking Intersections (Fluid Borders)
            float r = 20.0f; // Radius
            float lx = pOffset + parkingRoads.leftX;
            float rx = pOffset + parkingRoads.rightX;
            float ty = parkingRoads.topLimitY + hRoadH; // Bottom of top lane
            float my_top = parkingRoads.middle_SharedOuterY - hRoadH / 2;
            float my_bot = parkingRoads.middle_SharedOuterY + hRoadH / 2;
            float by = parkingRoads.row4_OuterY - 20;

            // 1. Vertical Lines
            float l_left = lx - vRoadW / 2;
            float l_right = lx + vRoadW / 2;
            DrawLineEx({l_left, ty + r}, {l_left, my_top - r}, thick, w);
            DrawLineEx({l_right, ty + r}, {l_right, my_top - r}, thick, w);
            DrawLineEx({l_left, my_bot + r}, {l_left, by - r}, thick, w);
            DrawLineEx({l_right, my_bot + r}, {l_right, by - r}, thick, w);

            float r_left = rx - vRoadW / 2;
            float r_right = rx + vRoadW / 2;
            DrawLineEx({r_left, ty + r}, {r_left, my_top - r}, thick, w);
            DrawLineEx({r_right, ty + r}, {r_right, my_top - r}, thick, w);
            DrawLineEx({r_left, my_bot + r}, {r_left, by - r}, thick, w);
            DrawLineEx({r_right, my_bot + r}, {r_right, by - r}, thick, w);

            // 2. Horizontal Lines
            DrawLineEx({pOffset + 20, ty}, {l_left - r, ty}, thick, w);
            DrawLineEx({l_right + r, ty}, {r_left - r, ty}, thick, w);
            DrawLineEx({r_right + r, ty}, {pOffset + 1180, ty}, thick, w);

            DrawLineEx({pOffset + 20, my_top}, {l_left - r, my_top}, thick, w);
            DrawLineEx({l_right + r, my_top}, {r_left - r, my_top}, thick, w);
            DrawLineEx({r_right + r, my_top}, {pOffset + 1180, my_top}, thick, w);

            DrawLineEx({pOffset + 20, my_bot}, {l_left - r, my_bot}, thick, w);
            DrawLineEx({l_right + r, my_bot}, {r_left - r, my_bot}, thick, w);
            DrawLineEx({r_right + r, my_bot}, {pOffset + 1180, my_bot}, thick, w);

            DrawLineEx({pOffset + 20, by}, {l_left - r, by}, thick, w);
            DrawLineEx({l_right + r, by}, {r_left - r, by}, thick, w);
            DrawLineEx({r_right + r, by}, {pOffset + 1180, by}, thick, w);

            // 3. CURVES
            DrawRoadCurve(l_left, ty, r, 0);
            DrawRoadCurve(l_right, ty, r, 1);
            DrawRoadCurve(l_left, my_top, r, 3);
            DrawRoadCurve(l_right, my_top, r, 2);
            DrawRoadCurve(l_left, my_bot, r, 0);
            DrawRoadCurve(l_right, my_bot, r, 1);
            DrawRoadCurve(l_left, by, r, 3);
            DrawRoadCurve(l_right, by, r, 2);

            DrawRoadCurve(r_left, ty, r, 0);
            DrawRoadCurve(r_right, ty, r, 1);
            DrawRoadCurve(r_left, my_top, r, 3);
            DrawRoadCurve(r_right, my_top, r, 2);
            DrawRoadCurve(r_left, my_bot, r, 0);
            DrawRoadCurve(r_right, my_bot, r, 1);
            DrawRoadCurve(r_left, by, r, 3);
            DrawRoadCurve(r_right, by, r, 2);

            // Yellow Dividers
            DrawRectangle(pOffset + 20, startY_Upper_Bottom - 2, 1160, 4, Colors::STOPPER);
            DrawRectangle(pOffset + 20, startY_Bottom - 2, 1160, 4, Colors::STOPPER);

            // Objects
            // Pass the vector of textures to the station draw function
            chargingStation.Draw(carTextures);
            for (auto &spot : parkingSpots)
                spot.Draw(parkingManager.IsCharger(spot.GetID()), parkingManager.GetPrice(spot.GetID()));

            // Draw parking cars with correct random texture
            for (auto &car : parkingCars)
            {
                if (car.textureVariant >= 0 && car.textureVariant < carTextures.size())
                    car.DrawSmart(carTextures[car.textureVariant]);
                else
                    car.DrawSmart(carTextures[0]);
            }

            EndMode2D();
            DrawText(TextFormat("Parking Cars: %d", (int)parkingCars.size()), 10, 10, 20, WHITE);
        }

        EndDrawing();
    }

    // Unload all textures
    for (auto &t : carTextures)
        UnloadTexture(t);
    UnloadTexture(menuBg);
    CloseWindow();
    return 0;
}