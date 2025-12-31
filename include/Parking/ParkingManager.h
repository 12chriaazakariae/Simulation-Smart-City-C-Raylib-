#ifndef PARKING_MANAGER_H
#define PARKING_MANAGER_H

#include "../Config/Shared.h"
#include "ParkingSpot.h"
#include "../Car/SmartCar.h"

// Manages the logic for assigning parking spots to cars based on budget and availability
class ParkingManager
{
private:
    vector<ParkingSpot> *spots;
    vector<int> chargerIDs;

public:
    ParkingManager(vector<ParkingSpot> *s, vector<int> chargers) : spots(s), chargerIDs(chargers) {}

    // Main algorithm: Finds the best available spot a car can afford
    // Prioritizes expensive/premium spots first if the car has the budget
    ParkingSpot *RequestBestSpot(Vector2 carPos, bool isElectric, bool needsCharging, float carBudget, const vector<SmartParkingCar> &cars)
    {
        // Spot IDs grouped by price tiers/location
        vector<vector<int>> parkingPriorities = {
            {20, 19, 18, 17, 16, 15, 14, 13, 12, 11}, // Tier 1 ($30)
            {10, 9, 8, 7, 6, 5, 4, 3, 2, 1},          // Tier 2 ($10)
            {21, 22, 23, 24, 25, 26, 27, 28, 29, 30}, // Tier 3 ($5)
            {31, 32, 33, 34, 35, 36, 37, 38, 39, 40}  // Tier 4 ($20)
        };

        // Helper to map a price to the priority list index above
        auto GetPriorityListIndex = [](float price) -> int
        {
            if (price == 30.0f)
                return 0;
            if (price == 10.0f)
                return 1;
            if (price == 5.0f)
                return 2;
            if (price == 20.0f)
                return 3;
            return -1;
        };

        // Helper to check if a spot is physically empty and not reserved by another car
        auto IsSpotAvailable = [this, &cars](int spotID) -> bool
        {
            ParkingSpot *spot = this->GetSpotByID(spotID);
            if (!spot || spot->IsOccupied() || spot->IsReserved())
                return false;

            // Double check against active car targets
            for (const auto &car : cars)
                if (car.GetAssignedSpotID() == spotID)
                    return false;
            return true;
        };

        // filter prices that the car can actually afford
        vector<float> affordablePrices;
        vector<float> allPrices = {30.0f, 20.0f, 10.0f, 5.0f};

        for (float price : allPrices)
            if (carBudget >= price)
                affordablePrices.push_back(price);

        // Iterate through affordable tiers to find the first free spot
        for (float price : affordablePrices)
        {
            int listIndex = GetPriorityListIndex(price);
            if (listIndex == -1)
                continue;

            for (int spotID : parkingPriorities[listIndex])
                if (IsSpotAvailable(spotID))
                    return GetSpotByID(spotID);
        }

        return nullptr; // No suitable spot found
    }

    // Helper to retrieve a spot object by its numeric ID
    ParkingSpot *GetSpotByID(int id)
    {
        for (auto &spot : *spots)
            if (spot.GetID() == id)
                return &spot;
        return nullptr;
    }

    // Checks if a specific spot ID is equipped with an EV charger
    bool IsCharger(int id)
    {
        for (int c : chargerIDs)
            if (c == id)
                return true;
        return false;
    }

    // Returns the cost per session for a specific spot based on its ID range
    float GetPrice(int id)
    {
        if (id >= 1 && id <= 10)
            return 10.0f;
        if (id >= 11 && id <= 20)
            return 30.0f;
        if (id >= 21 && id <= 30)
            return 5.0f;
        if (id >= 31 && id <= 40)
            return 20.0f;
        return 0.0f;
    }
};

#endif // PARKING_MANAGER_H