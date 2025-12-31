#ifndef PARKING_SPOT_H
#define PARKING_SPOT_H

#include "../Config/Shared.h"

// Represents a single parking space in the Smart Parking Lot (Zone 2)
// Handles its own state (Occupied/Reserved) and rendering logic
class ParkingSpot
{
private:
    Rectangle rect;
    bool occupied, reserved;
    int id;
    bool facesDown; // Determines which end of the spot is closed
    float lineThickness, padding;

public:
    ParkingSpot(int identifier, float x, float y, float w, float h, bool openDirectionDown)
    {
        id = identifier;
        rect = {x + Config::PARKING_OFFSET_X, y, w, h};
        occupied = false;
        reserved = false;
        facesDown = openDirectionDown;
        lineThickness = 3.0f;
        padding = 5.0f;
    }

    // Getters for AI navigation and collision detection
    Vector2 GetCenter() const { return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f}; }
    int GetID() const { return id; }
    Rectangle GetRect() const { return rect; }
    bool GetFacesDown() const { return facesDown; }

    // State setters
    void SetOccupied(bool status) { occupied = status; }
    void SetReserved(bool status) { reserved = status; }
    bool IsOccupied() const { return occupied; }
    bool IsReserved() const { return reserved; }

    // Renders the spot, including boundary lines, status indicators, and price tags
    void Draw(bool isCharger, float price)
    {
        // 1. Draw the Spot Floor
        DrawRectangleRec(rect, Colors::ASPHALT);

        // 2. Draw Boundary Lines
        float left = rect.x;
        float right = rect.x + rect.width;
        float top = rect.y;
        float bottom = rect.y + rect.height;

        // Side dividers
        DrawLineEx({left, top}, {left, bottom}, lineThickness, Colors::LINES);
        DrawLineEx({right, top}, {right, bottom}, lineThickness, Colors::LINES);

        // Backstop line (Closed end depends on orientation)
        if (facesDown)
            DrawLineEx({left, top}, {right, top}, lineThickness, Colors::LINES);
        else
            DrawLineEx({left, bottom}, {right, bottom}, lineThickness, Colors::LINES);

        // 3. Status Indicator Strip (Green=Free, Red=Occupied, Orange=Reserved)
        Color statusColor = GREEN;
        if (occupied)
            statusColor = RED;
        else if (reserved)
            statusColor = ORANGE;

        float stripHeight = 5.0f;
        if (facesDown)
            DrawRectangle(left + 2, top + 2, rect.width - 4, stripHeight, statusColor);
        else
            DrawRectangle(left + 2, bottom - stripHeight - 2, rect.width - 4, stripHeight, statusColor);

        // 4. Info Text (ID and Price)
        DrawText(TextFormat("%d", id), (int)(rect.x + 5), (int)(rect.y + rect.height / 2 - 10), 20, LIGHTGRAY);

        // Color code price for visual tiering
        Color priceColor = (price >= 20) ? PURPLE : SKYBLUE;
        DrawText(TextFormat("$%.0f", price), (int)(rect.x + rect.width - 35), (int)(rect.y + rect.height / 2 - 5), 10, priceColor);

        // Optional EV label if this spot has a charger
        if (isCharger)
            DrawText("EV", (int)(rect.x + 5), (int)(rect.y + rect.height * 0.7f), 10, GREEN);
    }
};

#endif // PARKING_SPOT_H