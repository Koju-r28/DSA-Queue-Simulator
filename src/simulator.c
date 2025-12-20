#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "traffic_simulator.h"

// Global queues for lanes
Queue laneQueues[4]; // Queues for lanes A, B, C, D
int lanePriorities[4] = {0}; // Priority levels for lanes (0 = normal, 1 = high)

const SDL_Color VEHICLE_COLORS[] = {
    {50, 50, 200, 255},    // REGULAR_CAR: Blue (FIXED from black)
    {255, 0, 0, 255}       // EMERGENCY_VEHICLE: Red
};

void initializeTrafficLights(TrafficLight *lights) {
    lights[DIRECTION_NORTH] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, 
                    INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT,
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH
    };

    lights[DIRECTION_SOUTH] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, 
                    INTERSECTION_Y + LANE_WIDTH,
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH
    };

    lights[DIRECTION_EAST] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH, 
                    INTERSECTION_Y - LANE_WIDTH,
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST
    };

    lights[DIRECTION_WEST] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, 
                    INTERSECTION_Y - LANE_WIDTH,
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST
    };
}

void updateTrafficLights(TrafficLight *lights) {
    Uint32 currentTicks = SDL_GetTicks();
    static Uint32 lastUpdateTicks = 0;

    if (currentTicks - lastUpdateTicks >= 8000) { // FIXED: Changed to 8 seconds (was too long at 10)
        lastUpdateTicks = currentTicks;
        int maxPriorityLane = -1;

        // Update priority status of lanes
        for (int i = 0; i < 4; i++) {
            if (laneQueues[i].size > 10) {
                lanePriorities[i] = 1; // Set high priority
                if (maxPriorityLane == -1 || laneQueues[i].size > laneQueues[maxPriorityLane].size) {
                    maxPriorityLane = i;
                }
            } else if (laneQueues[i].size < 5) {
                lanePriorities[i] = 0; // Reset priority
            }
        }

        // Assign green light based on priority
        if (maxPriorityLane != -1) {
            for (int i = 0; i < 4; i++) {
                lights[i].state = (i == maxPriorityLane) ? GREEN : RED;
            }
        } else {
            // FIXED: Only one direction gets green at a time
            static int normalCycle = 0;
            
            // Set all to red first
            for (int i = 0; i < 4; i++) {
                lights[i].state = RED;
            }
            
            // Give green to one direction
            switch (normalCycle % 4) {
                case 0:
                    lights[DIRECTION_NORTH].state = GREEN;
                    break;
                case 1:
                    lights[DIRECTION_EAST].state = GREEN;
                    break;
                case 2:
                    lights[DIRECTION_SOUTH].state = GREEN;
                    break;
                case 3:
                    lights[DIRECTION_WEST].state = GREEN;
                    break;
            }
            normalCycle++;
        }
    }
}

Vehicle *createVehicle(Direction direction) {
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    if (!vehicle) return NULL;  // FIXED: Check allocation
    
    vehicle->direction = direction;
    vehicle->type = REGULAR_CAR;
    vehicle->active = true;
    vehicle->speed = 2.0f;
    vehicle->state = STATE_MOVING;
    vehicle->turnAngle = 0.0f;
    vehicle->turnProgress = 0.0f;
    vehicle->isInRightLane = (rand() % 2 == 0);  // FIXED: Properly initialize

    // FIXED: Initialize turn direction randomly
    int turnChoice = rand() % 10;
    if (turnChoice < 2) {  // 20% turn left
        vehicle->turnDirection = TURN_LEFT;
    } else if (turnChoice < 4) {  // 20% turn right
        vehicle->turnDirection = TURN_RIGHT;
    } else {  // 60% go straight
        vehicle->turnDirection = TURN_STRAIGHT;
    }

    // Set vehicle dimensions based on direction
    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH) {
        vehicle->rect.w = 20;
        vehicle->rect.h = 30;
    } else {
        vehicle->rect.w = 30;
        vehicle->rect.h = 20;
    }

    // Determine lane position
    int laneOffset = vehicle->isInRightLane ? 0 : 1;
    
    switch (direction) {
        case DIRECTION_NORTH:
            vehicle->y = WINDOW_HEIGHT + 50;  // Start off-screen
            vehicle->x = INTERSECTION_X + LANE_WIDTH / 4 + (laneOffset * LANE_WIDTH / 2);
            break;
        case DIRECTION_SOUTH:
            vehicle->y = -50;  // Start off-screen
            vehicle->x = INTERSECTION_X - LANE_WIDTH / 4 - (laneOffset * LANE_WIDTH / 2);
            break;
        case DIRECTION_EAST:
            vehicle->x = -50;  // Start off-screen
            vehicle->y = INTERSECTION_Y + LANE_WIDTH / 4 + (laneOffset * LANE_WIDTH / 2);
            break;
        case DIRECTION_WEST:
            vehicle->x = WINDOW_WIDTH + 50;  // Start off-screen
            vehicle->y = INTERSECTION_Y - LANE_WIDTH / 4 - (laneOffset * LANE_WIDTH / 2);
            break;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    return vehicle;
}

void updateVehicle(Vehicle *vehicle, TrafficLight *lights) {
    if (!vehicle->active) return;

    float stopLine = 0;
    bool shouldStop = false;
    float stopDistance = 50.0f;  // FIXED: Increased from 40 for better stopping

    // Calculate stop lines
    switch (vehicle->direction) {
        case DIRECTION_NORTH:
            stopLine = INTERSECTION_Y + LANE_WIDTH + 10;
            break;
        case DIRECTION_SOUTH:
            stopLine = INTERSECTION_Y - LANE_WIDTH - 10;
            break;
        case DIRECTION_EAST:
            stopLine = INTERSECTION_X - LANE_WIDTH - 10;
            break;
        case DIRECTION_WEST:
            stopLine = INTERSECTION_X + LANE_WIDTH + 10;
            break;
    }

    // Check if vehicle should stop (left turns allowed on red)
    if (vehicle->turnDirection != TURN_LEFT && vehicle->state != STATE_TURNING) {
        switch (vehicle->direction) {
            case DIRECTION_NORTH:
                shouldStop = (vehicle->y > stopLine - stopDistance) && 
                           (vehicle->y < stopLine + 10) && 
                           lights[DIRECTION_NORTH].state == RED;
                break;
            case DIRECTION_SOUTH:
                shouldStop = (vehicle->y < stopLine + stopDistance) && 
                           (vehicle->y > stopLine - 10) && 
                           lights[DIRECTION_SOUTH].state == RED;
                break;
            case DIRECTION_EAST:
                shouldStop = (vehicle->x < stopLine + stopDistance) && 
                           (vehicle->x > stopLine - 10) && 
                           lights[DIRECTION_EAST].state == RED;
                break;
            case DIRECTION_WEST:
                shouldStop = (vehicle->x > stopLine - stopDistance) && 
                           (vehicle->x < stopLine + 10) && 
                           lights[DIRECTION_WEST].state == RED;
                break;
        }
    }

    // State management
    if (shouldStop) {
        vehicle->state = STATE_STOPPING;
        vehicle->speed *= 0.85f;
        if (vehicle->speed < 0.1f) {
            vehicle->state = STATE_STOPPED;
            vehicle->speed = 0;
        }
    } else if (vehicle->state == STATE_STOPPED && !shouldStop) {
        vehicle->state = STATE_MOVING;
        vehicle->speed = 2.0f;
    }

    // Movement
    if (vehicle->state == STATE_MOVING || vehicle->state == STATE_STOPPING || vehicle->state == STATE_TURNING) {
        switch (vehicle->direction) {
            case DIRECTION_NORTH:
                vehicle->y -= vehicle->speed;
                break;
            case DIRECTION_SOUTH:
                vehicle->y += vehicle->speed;
                break;
            case DIRECTION_EAST:
                vehicle->x += vehicle->speed;
                break;
            case DIRECTION_WEST:
                vehicle->x -= vehicle->speed;
                break;
        }
    }

    // Simple turn execution at intersection
    bool atIntersection = false;
    switch (vehicle->direction) {
        case DIRECTION_NORTH:
            atIntersection = (vehicle->y <= INTERSECTION_Y && vehicle->y >= INTERSECTION_Y - 20);
            break;
        case DIRECTION_SOUTH:
            atIntersection = (vehicle->y >= INTERSECTION_Y && vehicle->y <= INTERSECTION_Y + 20);
            break;
        case DIRECTION_EAST:
            atIntersection = (vehicle->x >= INTERSECTION_X && vehicle->x <= INTERSECTION_X + 20);
            break;
        case DIRECTION_WEST:
            atIntersection = (vehicle->x <= INTERSECTION_X && vehicle->x >= INTERSECTION_X - 20);
            break;
    }

    if (atIntersection && vehicle->state != STATE_TURNING && vehicle->turnDirection != TURN_STRAIGHT) {
        vehicle->state = STATE_TURNING;
        
        if (vehicle->turnDirection == TURN_LEFT) {
            vehicle->direction = (Direction)((vehicle->direction + 3) % 4);
        } else if (vehicle->turnDirection == TURN_RIGHT) {
            vehicle->direction = (Direction)((vehicle->direction + 1) % 4);
        }
        
        // Adjust dimensions after turn
        if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH) {
            vehicle->rect.w = 20;
            vehicle->rect.h = 30;
        } else {
            vehicle->rect.w = 30;
            vehicle->rect.h = 20;
        }
        
        vehicle->turnDirection = TURN_STRAIGHT;
        vehicle->state = STATE_MOVING;
    }

    // Remove vehicle if out of bounds
    if (vehicle->x < -100 || vehicle->x > WINDOW_WIDTH + 100 ||
        vehicle->y < -100 || vehicle->y > WINDOW_HEIGHT + 100) {
        vehicle->active = false;
    }

    // Update rectangle
    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;
}

void renderRoads(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    int laneWidth = LANE_WIDTH / 2;

    // Draw intersection
    SDL_Rect intersection = {
        INTERSECTION_X - (laneWidth * 2), 
        INTERSECTION_Y - (laneWidth * 2),
        laneWidth * 4, 
        laneWidth * 4
    };
    SDL_RenderFillRect(renderer, &intersection);

    // Draw roads
    SDL_Rect verticalRoad1 = {
        INTERSECTION_X - (laneWidth * 2), 0, 
        laneWidth * 4, INTERSECTION_Y - (laneWidth * 2)
    };
    SDL_Rect verticalRoad2 = {
        INTERSECTION_X - (laneWidth * 2), 
        INTERSECTION_Y + (laneWidth * 2),
        laneWidth * 4, 
        WINDOW_HEIGHT - INTERSECTION_Y - (laneWidth * 2)
    };
    SDL_Rect horizontalRoad1 = {
        0, INTERSECTION_Y - (laneWidth * 2), 
        INTERSECTION_X - (laneWidth * 2), laneWidth * 4
    };
    SDL_Rect horizontalRoad2 = {
        INTERSECTION_X + (laneWidth * 2), 
        INTERSECTION_Y - (laneWidth * 2),
        WINDOW_WIDTH - INTERSECTION_X - (laneWidth * 2), 
        laneWidth * 4
    };

    SDL_RenderFillRect(renderer, &verticalRoad1);
    SDL_RenderFillRect(renderer, &verticalRoad2);
    SDL_RenderFillRect(renderer, &horizontalRoad1);
    SDL_RenderFillRect(renderer, &horizontalRoad2);

    // Lane dividers
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < WINDOW_HEIGHT; i += 40) {
        if (i < INTERSECTION_Y - (laneWidth * 2) || i > INTERSECTION_Y + (laneWidth * 2)) {
            for (int j = -laneWidth * 2 + laneWidth; j < laneWidth * 2; j += laneWidth) {
                SDL_Rect div = {INTERSECTION_X + j - 1, i, 2, 20};
                SDL_RenderFillRect(renderer, &div);
            }
        }
    }

    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        if (i < INTERSECTION_X - (laneWidth * 2) || i > INTERSECTION_X + (laneWidth * 2)) {
            for (int j = -laneWidth * 2 + laneWidth; j < laneWidth * 2; j += laneWidth) {
                SDL_Rect div = {i, INTERSECTION_Y + j - 1, 20, 2};
                SDL_RenderFillRect(renderer, &div);
            }
        }
    }

    // Stop lines
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    int lineWidth = STOP_LINE_WIDTH;

    SDL_Rect stops[4] = {
        {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2) - lineWidth, laneWidth * 4, lineWidth},
        {INTERSECTION_X - (laneWidth * 2), INTERSECTION_Y + (laneWidth * 2), laneWidth * 4, lineWidth},
        {INTERSECTION_X + (laneWidth * 2), INTERSECTION_Y - (laneWidth * 2), lineWidth, laneWidth * 4},
        {INTERSECTION_X - (laneWidth * 2) - lineWidth, INTERSECTION_Y - (laneWidth * 2), lineWidth, laneWidth * 4}
    };

    for (int i = 0; i < 4; i++) {
        SDL_RenderFillRect(renderer, &stops[i]);
    }
}

void renderQueues(SDL_Renderer *renderer) {
    const char* labels[] = {"N", "S", "E", "W"};
    
    for (int i = 0; i < 4; i++) {
        int x = 10 + i * 150;
        int y = 10;
        
        // Draw queue box
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 200);
        SDL_Rect queueBox = {x - 5, y - 5, 50, 40 + laneQueues[i].size * 8};
        SDL_RenderFillRect(renderer, &queueBox);
        
        // Draw vehicles in queue
        Node *current = laneQueues[i].front;
        int count = 0;
        while (current != NULL && count < 20) {
            SDL_Rect vehicleRect = {x, y + count * 8, 40, 6};
            SDL_SetRenderDrawColor(renderer, 50, 50, 200, 255);
            SDL_RenderFillRect(renderer, &vehicleRect);
            current = current->next;
            count++;
        }
    }
}

void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats) {
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);

    renderRoads(renderer);

    // Render traffic lights
    for (int i = 0; i < 4; i++) {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &lights[i].position);
        
        SDL_Color lightColor;
        if (lights[i].state == RED) {
            lightColor = (SDL_Color){255, 0, 0, 255};
        } else if (lights[i].state == GREEN) {
            lightColor = (SDL_Color){0, 255, 0, 255};
        } else {
            lightColor = (SDL_Color){255, 255, 0, 255};
        }
        
        SDL_SetRenderDrawColor(renderer, lightColor.r, lightColor.g, lightColor.b, lightColor.a);
        SDL_RenderFillRect(renderer, &lights[i].position);
    }

    // Render vehicles
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) {
            SDL_Color color = VEHICLE_COLORS[vehicles[i].type];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &vehicles[i].rect);
        }
    }

    renderQueues(renderer);
    SDL_RenderPresent(renderer);
}

// Queue functions
void initQueue(Queue *q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue *q, Vehicle vehicle) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) return;  // FIXED: Check allocation
    
    newNode->vehicle = vehicle;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
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
    
    return vehicle;
}

int isQueueEmpty(Queue *q) {
    return q->front == NULL;
}

// Utility functions
void updateStatistics(Statistics *stats, Vehicle *vehicles) {
    int activeCount = 0;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) activeCount++;
    }
    stats->totalVehicles = activeCount;
}

void cleanupInactiveVehicles(Vehicle *vehicles) {
    // Mark inactive vehicles for reuse
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i] = (Vehicle){0};  // Reset
        }
    }
}