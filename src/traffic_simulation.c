#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"


// Global queues for lanes
Queue laneQueues[4];         // Queues for lanes A, B, C, D
int lanePriorities[4] = {0}; // Priority levels for lanes (0 = normal, 1 = high)

const SDL_Color VEHICLE_COLORS[] = {
    {0, 0, 0, 255}, // REGULAR_CAR: Blue
};

void initializeTrafficLights(TrafficLight *lights)
{
    lights[0] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH};
    lights[1] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH};
    lights[2] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST};
    lights[3] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST};
}

void updateTrafficLights(TrafficLight *lights) {
    Uint32 currentTicks = SDL_GetTicks();
    static Uint32 lastUpdateTicks = 0;

    if (currentTicks - lastUpdateTicks >= 10000) { // Change every 10 seconds
        lastUpdateTicks = currentTicks;

        int maxPriorityLane = -1;  // Track highest priority lane

        // Update priority status of lanes
        for (int i = 0; i < 4; i++) {
            if (laneQueues[i].size > 10) {
                lanePriorities[i] = 1; // Set high priority
                if (maxPriorityLane == -1 || laneQueues[i].size > laneQueues[maxPriorityLane].size) {
                    maxPriorityLane = i; // Find the lane with the most vehicles
                }
            } else if (laneQueues[i].size < 5) {
                lanePriorities[i] = 0; // Reset priority
            }
        }

        // Assign green light only to the highest priority lane
        for (int i = 0; i < 4; i++) {
            lights[i].state = (i == maxPriorityLane) ? GREEN : RED;
        }

        // If no priority lane, alternate between A&C or B&D
        static int normalCycle = 0;
        if (maxPriorityLane == -1) {
            if (normalCycle % 2 == 0) {
                lights[DIRECTION_NORTH].state = GREEN;
                lights[DIRECTION_SOUTH].state = GREEN;
                lights[DIRECTION_EAST].state = RED;
                lights[DIRECTION_WEST].state = RED;
            } else {
                lights[DIRECTION_EAST].state = GREEN;
                lights[DIRECTION_WEST].state = GREEN;
                lights[DIRECTION_NORTH].state = RED;
                lights[DIRECTION_SOUTH].state = RED;
            }
            normalCycle++;
        }
    }
}

Vehicle *createVehicle(Direction direction)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;

    // Only spawn REGULAR_CAR (No emergency vehicles)
    vehicle->type = REGULAR_CAR;
    vehicle->active = true;
    vehicle->speed = 2.0f; // Default speed for regular cars
    vehicle->state = STATE_MOVING;
    vehicle->turnAngle = 0.0f;
    vehicle->turnProgress = 0.0f;

    // Set vehicle dimensions based on direction
    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH)
    {
        vehicle->rect.w = 20; // width
        vehicle->rect.h = 30; // height
    }
    else
    {
        vehicle->rect.w = 30; // width
        vehicle->rect.h = 20; // height
    }

    // Determine incoming or outgoing lane
    int laneOffset = rand() % 2; // Choose either of the two incoming lanes

    switch (direction)
    {
    case DIRECTION_NORTH: // Incoming from bottom, outgoing to top
        vehicle->y = (laneOffset == 0) ? WINDOW_HEIGHT - LANE_WIDTH / 4 : WINDOW_HEIGHT - (LANE_WIDTH / 4) * 2;
        vehicle->x = INTERSECTION_X + LANE_WIDTH / 2; // Right-side incoming lane
        break;

    case DIRECTION_SOUTH: // Incoming from top, outgoing to bottom
        vehicle->y = (laneOffset == 0) ? 0 + LANE_WIDTH / 4 : (LANE_WIDTH / 4) * 2;
        vehicle->x = INTERSECTION_X - LANE_WIDTH / 2; // Right-side incoming lane
        break;

    case DIRECTION_EAST: // Incoming from left, outgoing to right
        vehicle->x = (laneOffset == 0) ? 0 + LANE_WIDTH / 4 : (LANE_WIDTH / 4) * 2;
        vehicle->y = INTERSECTION_Y + LANE_WIDTH / 2; // Right-side incoming lane
        break;

    case DIRECTION_WEST: // Incoming from right, outgoing to left
        vehicle->x = (laneOffset == 0) ? WINDOW_WIDTH - LANE_WIDTH / 4 : WINDOW_WIDTH - (LANE_WIDTH / 4) * 2;
        vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2; // Right-side incoming lane
        break;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    return vehicle;
}

void updateVehicle(Vehicle *vehicle, TrafficLight *lights)
{
    if (!vehicle->active)
        return;

    float stopLine = 0;
    bool shouldStop = false;
    float stopDistance = 40.0f;
    float turnPoint = 0;

    // Step 1: Calculate Stop Lines & Turn Points
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        stopLine = INTERSECTION_Y + LANE_WIDTH + 40;
        turnPoint = INTERSECTION_Y;
        break;
    case DIRECTION_SOUTH:
        stopLine = INTERSECTION_Y - LANE_WIDTH - 40;
        turnPoint = INTERSECTION_Y;
        break;
    case DIRECTION_EAST:
        stopLine = INTERSECTION_X - LANE_WIDTH - 40;
        turnPoint = INTERSECTION_X;
        break;
    case DIRECTION_WEST:
        stopLine = INTERSECTION_X + LANE_WIDTH + 40;
        turnPoint = INTERSECTION_X;
        break;
    }

    // Step 2: Check if the vehicle should stop at a red light (EXCEPT for Left Turns)
    if (vehicle->turnDirection != TURN_LEFT) // 🚦 If NOT turning left, follow the light
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            shouldStop = (vehicle->y > stopLine - stopDistance) &&
                         (vehicle->y < stopLine) &&
                         lights[DIRECTION_NORTH].state == RED;
            break;
        case DIRECTION_SOUTH:
            shouldStop = (vehicle->y < stopLine + stopDistance) &&
                         (vehicle->y > stopLine) &&
                         lights[DIRECTION_SOUTH].state == RED;
            break;
        case DIRECTION_EAST:
            shouldStop = (vehicle->x < stopLine + stopDistance) &&
                         (vehicle->x > stopLine) &&
                         lights[DIRECTION_EAST].state == RED;
            break;
        case DIRECTION_WEST:
            shouldStop = (vehicle->x > stopLine - stopDistance) &&
                         (vehicle->x < stopLine) &&
                         lights[DIRECTION_WEST].state == RED;
            break;
        }
    }
    else
    {
        shouldStop = false; // 🚦 Left turns are ALLOWED on red
    }

    // Step 3: Stop or Move
    if (shouldStop)
    {
        vehicle->state = STATE_STOPPING;
        vehicle->speed *= 0.8f;
        if (vehicle->speed < 0.1f)
        {
            vehicle->state = STATE_STOPPED;
            vehicle->speed = 0;
        }
    }
    else if (vehicle->state == STATE_STOPPED && !shouldStop)
    {
        vehicle->state = STATE_MOVING;
        vehicle->speed = 2.0f; // Default speed for regular cars
    }

    // Step 4: Movement Logic
    if (vehicle->state == STATE_MOVING || vehicle->state == STATE_STOPPING)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->y -= vehicle->speed; // Move UP
            break;
        case DIRECTION_SOUTH:
            vehicle->y += vehicle->speed; // Move DOWN
            break;
        case DIRECTION_EAST:
            vehicle->x += vehicle->speed; // Move RIGHT
            break;
        case DIRECTION_WEST:
            vehicle->x -= vehicle->speed; // Move LEFT
            break;
        }
    }

    // Step 5: Remove vehicle if out of screen
    if ((vehicle->direction == DIRECTION_NORTH && vehicle->y < -50) ||
        (vehicle->direction == DIRECTION_SOUTH && vehicle->y > WINDOW_HEIGHT + 50) ||
        (vehicle->direction == DIRECTION_EAST && vehicle->x > WINDOW_WIDTH + 50) ||
        (vehicle->direction == DIRECTION_WEST && vehicle->x < -50))
    {
        vehicle->active = false; // Remove vehicle after it exits
    }

    // Step 6: Update vehicle rectangle
    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;
}




void renderRoads(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray color for roads

    // Define lane width division (4 lanes per road)
    int laneWidth = LANE_WIDTH / 2; 

    // Draw the intersection
    SDL_Rect intersection = {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2), laneWidth * 4, laneWidth * 4};
    SDL_RenderFillRect(renderer, &intersection);

    // Draw main roads (4-lane division)
    SDL_Rect verticalRoad1 = {INTERSECTION_X - (laneWidth * 2), 0, laneWidth * 4, INTERSECTION_Y - (laneWidth * 2)};
    SDL_Rect verticalRoad2 = {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y + (laneWidth * 2), laneWidth * 4, WINDOW_HEIGHT - INTERSECTION_Y - (laneWidth * 2)};
    SDL_Rect horizontalRoad1 = {0, INTERSECTION_Y - (laneWidth * 2), INTERSECTION_X - (laneWidth * 2), laneWidth * 4};
    SDL_Rect horizontalRoad2 = {INTERSECTION_X + (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2), WINDOW_WIDTH - INTERSECTION_X - (laneWidth * 2), laneWidth * 4};

    SDL_RenderFillRect(renderer, &verticalRoad1);
    SDL_RenderFillRect(renderer, &verticalRoad2);
    SDL_RenderFillRect(renderer, &horizontalRoad1);
    SDL_RenderFillRect(renderer, &horizontalRoad2);

    // Draw lane dividers
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White lane markers

    for (int i = 0; i < WINDOW_HEIGHT; i += 40)
    {
        if (i < INTERSECTION_Y - (laneWidth * 2) || i > INTERSECTION_Y + (laneWidth * 2))
        {
            for (int j = -laneWidth * 2 + laneWidth; j < laneWidth * 2; j += laneWidth)
            {
                SDL_Rect laneDivider = {INTERSECTION_X + j - 1, i, 2, 20};
                SDL_RenderFillRect(renderer, &laneDivider);
            }
        }
    }

    for (int i = 0; i < WINDOW_WIDTH; i += 40)
    {
        if (i < INTERSECTION_X - (laneWidth * 2) || i > INTERSECTION_X + (laneWidth * 2))
        {
            for (int j = -laneWidth * 2 + laneWidth; j < laneWidth * 2; j += laneWidth)
            {
                SDL_Rect laneDivider = {i, INTERSECTION_Y + j - 1, 20, 2};
                SDL_RenderFillRect(renderer, &laneDivider);
            }
        }
    }

    // **Make the stop lines thinner**
    SDL_SetRenderDrawColor(renderer, 1, 0, 0, 1); // Red stop lines
    int thinLineWidth = 1; // **Set a very thin stop line width (3 pixels)**

    SDL_Rect northStop = {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2) - thinLineWidth, laneWidth * 4, thinLineWidth};
    SDL_Rect southStop = {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y + (laneWidth * 2), laneWidth * 4, thinLineWidth};
    SDL_Rect eastStop = {INTERSECTION_X + (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2), thinLineWidth, laneWidth * 4};
    SDL_Rect westStop = {INTERSECTION_X - (laneWidth * 2) - thinLineWidth, INTERSECTION_Y - (laneWidth * 2), thinLineWidth, laneWidth * 4};

    SDL_RenderFillRect(renderer, &northStop);
    SDL_RenderFillRect(renderer, &southStop);
    SDL_RenderFillRect(renderer, &eastStop);
    SDL_RenderFillRect(renderer, &westStop);
}




void renderQueues(SDL_Renderer *renderer)
{
    for (int i = 0; i < 4; i++)
    {
        int x = 10 + i * 200; // Adjust position for each lane
        int y = 10;
        Node *current = laneQueues[i].front;
        while (current != NULL)
        {
            SDL_Rect vehicleRect = {x, y, 30, 30};
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for vehicles
            SDL_RenderFillRect(renderer, &vehicleRect);
            y += 40; // Move down for the next vehicle
            current = current->next;
        }
    }
}

void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats)
{
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);  // Sky Blue background
    SDL_RenderClear(renderer);

    // Render roads
    renderRoads(renderer);

    // Render traffic lights
    for (int i = 0; i < 4; i++)
    {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255); // Dark gray for housing
        SDL_RenderFillRect(renderer, &lights[i].position);
        SDL_SetRenderDrawColor(renderer, (lights[i].state == RED) ? 255 : 0, (lights[i].state == GREEN) ? 255 : 0, 0, 255);
        SDL_RenderFillRect(renderer, &lights[i].position);
    }

    // Render vehicles
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (vehicles[i].active)
        {
            SDL_SetRenderDrawColor(renderer, VEHICLE_COLORS[vehicles[i].type].r, VEHICLE_COLORS[vehicles[i].type].g, VEHICLE_COLORS[vehicles[i].type].b, VEHICLE_COLORS[vehicles[i].type].a);
            SDL_RenderFillRect(renderer, &vehicles[i].rect);
        }
    }

    // Render queues
    renderQueues(renderer);

    SDL_RenderPresent(renderer);
}

// Queue functions
void initQueue(Queue *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue *q, Vehicle vehicle)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->vehicle = vehicle;
    newNode->next = NULL;
    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

Vehicle dequeue(Queue *q) {
    if (q->front == NULL) {
        Vehicle emptyVehicle = {0};
        return emptyVehicle;
    }
    
    Node *temp = q->front;
    Vehicle vehicle = temp->vehicle;
    
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->size--;

    // Mark vehicle as inactive after it exits
    vehicle.active = false;
    return vehicle;
}


int isQueueEmpty(Queue *q)
{
    return q->front == NULL;
}