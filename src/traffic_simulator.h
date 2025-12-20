#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define LANE_WIDTH 80
#define MAX_VEHICLES 100  // FIXED: Increased from 10 to handle more vehicles
#define INTERSECTION_X (WINDOW_WIDTH / 2)
#define INTERSECTION_Y (WINDOW_HEIGHT / 2)  // FIXED: Added missing definition

typedef enum {
    DIRECTION_NORTH = 0,
    DIRECTION_SOUTH = 1,
    DIRECTION_EAST = 2,
    DIRECTION_WEST = 3
} Direction;

typedef enum {
    TURN_NONE = 0,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_STRAIGHT  // FIXED: Added for better turn logic
} TurnDirection;

typedef enum {
    STATE_MOVING,
    STATE_STOPPING,
    STATE_STOPPED,
    STATE_TURNING
} VehicleState;

typedef enum {
    REGULAR_CAR = 0,
    EMERGENCY_VEHICLE  // FIXED: Added for future expansion
} VehicleType;

typedef enum {
    RED,
    YELLOW,  // FIXED: Added yellow light state
    GREEN
} TrafficLightState;

#define TRAFFIC_LIGHT_WIDTH (LANE_WIDTH * 2)
#define TRAFFIC_LIGHT_HEIGHT (LANE_WIDTH - LANE_WIDTH / 3)
#define STOP_LINE_WIDTH 3  // FIXED: Changed from 5 to match rendering code

typedef struct {
    SDL_Rect rect;
    VehicleType type;
    Direction direction;
    TurnDirection turnDirection;
    VehicleState state;
    float speed;
    float x;
    float y;
    bool active;
    float turnAngle;  
    bool isInRightLane;
    float turnProgress;  // FIXED: Changed from bool to float for smoother turns
} Vehicle;

typedef struct {
    TrafficLightState state;
    int timer;
    SDL_Rect position;
    Direction direction;
} TrafficLight;

typedef struct {
    int vehiclesPassed;
    int totalVehicles;
    float averageWaitTime;  // FIXED: More useful than vehiclesPerMinute
    Uint32 startTime;
} Statistics;

// Queue data structure
typedef struct Node {
    Vehicle vehicle;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    int size;
} Queue;

// Declare laneQueues as an external variable
extern Queue laneQueues[4];

// Function declarations
void initializeTrafficLights(TrafficLight* lights);
void updateTrafficLights(TrafficLight* lights);
Vehicle* createVehicle(Direction direction);
void updateVehicle(Vehicle *vehicle, TrafficLight *lights);
void renderSimulation(SDL_Renderer* renderer, Vehicle* vehicles, TrafficLight* lights, Statistics* stats);
void renderRoads(SDL_Renderer* renderer);
void renderQueues(SDL_Renderer* renderer);

// Queue functions
void initQueue(Queue* q);
void enqueue(Queue* q, Vehicle vehicle);
Vehicle dequeue(Queue* q);
int isQueueEmpty(Queue* q);

// FIXED: Added missing utility functions
void updateStatistics(Statistics* stats, Vehicle* vehicles);
void cleanupInactiveVehicles(Vehicle* vehicles);

#endif // TRAFFIC_SIMULATION_H