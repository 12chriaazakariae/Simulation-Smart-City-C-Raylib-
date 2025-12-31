#ifndef SMART_CAR_H
#define SMART_CAR_H

#include "Car.h"

// Represents a vehicle specifically entering the EV Charging Station (Zone 1)
// Inherits all movement and pathfinding logic from ChargingCar
class SmartChargingCar : public ChargingCar
{
public:
    SmartChargingCar(int _id, Vector2 startPos, float _batt, float _budget) : ChargingCar(_id, startPos, true, _batt, _budget) {}
};

// Represents a vehicle operating within the Smart Parking Lot (Zone 2)
// Adds economic (budget) and electric (battery) properties to the standard ParkingCar
class SmartParkingCar : public ParkingCar
{
public:
    bool isElectric;
    float batteryLevel;
    bool needsCharging;
    float budget;
    bool hasPaid;

    // Constructor initializes EV status, budget, and determines car color based on wealth
    SmartParkingCar(float startX, float startY, bool electric, float carBudget) : ParkingCar(startX, startY, 0.0f)
    {
        isElectric = electric;
        budget = carBudget;

        batteryLevel = electric ? (float)GetRandomValue(10, 100) : 0.0f;
        needsCharging = (isElectric && batteryLevel < 30.0f);
        hasPaid = false;

        // Color coding helps visually identify high-budget cars
        if (budget >= 30.0f)
            color = GOLD;
        else if (budget >= 20.0f)
            color = PURPLE;
        else if (budget >= 10.0f)
            color = BLUE;
        else
            color = BROWN;
    }

    // Updates specific smart features: battery depletion and automatic payment
    void UpdateSmart(float priceOfSpot)
    {
        Update(); // Call base movement logic

        // Deplete battery while driving
        if (isElectric && !isParked && isActive)
        {
            batteryLevel -= 0.01f;
            if (batteryLevel < 30)
                needsCharging = true;
        }

        // Handle payment transaction once parked
        if (isParked && !hasPaid)
        {
            if (budget >= priceOfSpot)
            {
                budget -= priceOfSpot;
                hasPaid = true;
            }
        }

        // Reset payment status when leaving to allow future re-entry logic if needed
        if (isLeaving)
            hasPaid = false;
    }

    // Draws the car sprite along with HUD elements (Budget price, Battery bar)
    void DrawSmart(Texture2D sharedTexture)
    {
        Draw(sharedTexture); // Draw base car

        if (!isActive)
            return;

        Vector2 pos = GetPosition();

        // Draw Budget
        DrawText(TextFormat("$%.0f", budget), pos.x - 10, pos.y - 50, 10, DARKGREEN);

        // Draw Battery Bar if EV
        if (isElectric)
        {
            Color c = (needsCharging) ? RED : GREEN;
            DrawRectangle(pos.x - 20, pos.y - 40, 40, 5, DARKGRAY);                    // Background
            DrawRectangle(pos.x - 20, pos.y - 40, 40 * (batteryLevel / 100.0f), 5, c); // Level
            if (needsCharging)
                DrawText("⚡", pos.x + 15, pos.y - 50, 10, RED);
        }
    }
};

#endif // SMART_CAR_H