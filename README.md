# Simulation-Smart-City-C-Raylib-
**\***Project Description**\***

This project is a Smart City simulation module focused on Smart Parking management and Electric Vehicle (EV) Charging, developed in C++ using Raylib with a fully object-oriented architecture.

The system simulates:

Vehicle movement

Parking availability

EV charging stations

Real-time visualisation

Conflict management between vehicles

⚠️ Constraint respected:
No JSON or external configuration files — all configuration is hard-coded in C++.

📁 Project Structure

CPP-FINAL-PROJECT/
│
├── include/
│ ├── Car/
│ │ ├── Car.h
│ │ └── SmartCar.h
│ │
│ ├── Charging/
│ │ └── EvStation.h
│ │
│ ├── Parking/
│ │ ├── ParkingManager.h
│ │ └── ParkingSpot.h
│ │
│ └── Config/
│ └── Shared.h
│
├── SmartCity/
│ ├── assets/
│ │ └── cars/
│ │ ├── car.png
│ │ ├── car3.png
│ │ ├── car4.png
│ │ └── cars.png
│ │
│ ├── SmartParking.jpeg
│ └── SmartCity.exe
│
├── src/
│ └── SmartCity.cpp
│
├── docs/
│ └── Rapport.pdf  
│
├── Vid/
│ └── Video_Explicatif.mp4
│
└── README.md

🧠 Architecture Overview
🚗 Vehicles

Car : Base vehicle class

SmartCar : Electric vehicle with battery level and charging behavior

🅿️ Parking System

ParkingSpot : Represents a single parking place

ParkingManager : Handles parking allocation and conflict management

🔌 EV Charging

EvStation : Manages charging spots, queues, and charging time

⚙️ Configuration

Shared.h : Global constants and shared utilities

🧩 Design Patterns Used

-Strategy Pattern
Used for parking decision logic (vehicle chooses a parking spot).

-Observer Pattern
Used for dashboard/statistics updates (if enabled).

-State-based logic
Vehicles transition between:

-Driving

-Parking

-Charging

-Leaving

🎮 Visualisation

-Implemented using Raylib

-2D simulation

-Vehicles, parking spots, and EV stations rendered in real time

-Assets loaded from SmartCity/assets/

▶️ How to Run the Project
Option 1: Run the executable

Go to:
SmartCity/
RUN :
SmartCity.exe
