#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include <SDL2/SDL.h>
#include <stdbool.h>

// Window and lane configuration
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define LANE_WIDTH 80
#define MAX_VEHICLES 100
#define INTERSECTION_X (WINDOW_WIDTH / 2)
#define INTERSECTION_Y (WINDOW_HEIGHT / 2)

// Traffic light dimensions
#define TRAFFIC_LIGHT_WIDTH (LANE_WIDTH * 2)
#define TRAFFIC_LIGHT_HEIGHT 15

// Direction enumeration
typedef enum {
    DIRECTION_NORTH = 0,
    DIRECTION_SOUTH = 1,
    DIRECTION_EAST = 2,
    DIRECTION_WEST = 3
} Direction;

// Turn direction
typedef enum {
    TURN_STRAIGHT,
    TURN_LEFT,
    TURN_RIGHT
} TurnDirection;

// Vehicle state
typedef enum {
    STATE_MOVING,
    STATE_STOPPED
} VehicleState;

// Vehicle type
typedef enum {
    REGULAR_CAR
} VehicleType;

// Traffic light state
typedef enum {
    RED,
    GREEN
} TrafficLightState;

// Vehicle structure
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
    int colorIndex;
    bool shouldStopForVehicle;
    bool isTurning;
    float turnProgress;
    bool hasPassedCenter;
} Vehicle;

// Traffic light structure
typedef struct {
    TrafficLightState state;
    int timer;
    SDL_Rect position;
    Direction direction;
} TrafficLight;

// Statistics structure
typedef struct {
    int vehiclesPassed;
    int totalVehicles;
    float vehiclesPerMinute;
    Uint32 startTime;
} Statistics;

// Queue node structure
typedef struct Node {
    Vehicle vehicle;
    struct Node* next;
} Node;

// Queue structure
typedef struct {
    Node* front;
    Node* rear;
    int size;
} Queue;

// Global lane queues
extern Queue laneQueues[4];

// Traffic light functions
void initializeTrafficLights(TrafficLight* lights);
void updateTrafficLights(TrafficLight* lights);

// Vehicle functions
Vehicle* createVehicle(Direction direction);
void updateVehicle(Vehicle *vehicle, TrafficLight *lights);

// Collision detection
bool shouldStopForVehicleInQueue(Vehicle *vehicle, Direction lane);

// Rendering functions
void renderSimulation(SDL_Renderer* renderer, TrafficLight* lights, Statistics* stats);
void renderRoads(SDL_Renderer* renderer);

// Queue functions
void initQueue(Queue* q);
void enqueue(Queue* q, Vehicle vehicle);
Vehicle dequeue(Queue* q);
int isQueueEmpty(Queue* q);
void removeFromQueue(Queue* q, Vehicle* vehicle);

#endif // TRAFFIC_SIMULATION_H