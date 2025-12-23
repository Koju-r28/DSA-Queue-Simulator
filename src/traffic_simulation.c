#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"

// Global queues for lanes (one queue per direction)
Queue laneQueues[4];

// Pastel colors for vehicles
const SDL_Color VEHICLE_COLORS[] = {
    {255, 182, 193, 255},   // Light Pink
    {173, 216, 230, 255},   // Light Blue
    {144, 238, 144, 255},   // Light Green
    {255, 255, 224, 255},   // Light Yellow
    {221, 160, 221, 255},   // Plum
    {176, 224, 230, 255},   // Powder Blue
    {255, 218, 185, 255},   // Peach
    {216, 191, 216, 255},   // Thistle
};

void initializeTrafficLights(TrafficLight *lights) {
    lights[0] = (TrafficLight){
        .state = RED, .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, 
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH
    };
    
    lights[1] = (TrafficLight){
        .state = RED, .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, 
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH
    };
    
    lights[2] = (TrafficLight){
        .state = GREEN, .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, 
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST
    };
    
    lights[3] = (TrafficLight){
        .state = GREEN, .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, INTERSECTION_Y - LANE_WIDTH, 
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST
    };
}

void updateTrafficLights(TrafficLight *lights) {
    static Uint32 lastUpdate = 0;
    static int cycle = 0;
    Uint32 current = SDL_GetTicks();

    if (current - lastUpdate >= 5000) {
        lastUpdate = current;
        cycle = !cycle;

        if (cycle == 0) {
            lights[DIRECTION_NORTH].state = GREEN;
            lights[DIRECTION_SOUTH].state = GREEN;
            lights[DIRECTION_EAST].state = RED;
            lights[DIRECTION_WEST].state = RED;
        } else {
            lights[DIRECTION_NORTH].state = RED;
            lights[DIRECTION_SOUTH].state = RED;
            lights[DIRECTION_EAST].state = GREEN;
            lights[DIRECTION_WEST].state = GREEN;
        }
    }
}

Vehicle *createVehicle(Direction direction) {
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;
    vehicle->type = REGULAR_CAR;
    vehicle->active = true;
    vehicle->speed = 2.0f;
    vehicle->state = STATE_MOVING;
    vehicle->turnDirection = TURN_STRAIGHT;
    vehicle->colorIndex = rand() % 8;

    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH) {
        vehicle->rect.w = 20;
        vehicle->rect.h = 30;
    } else {
        vehicle->rect.w = 30;
        vehicle->rect.h = 20;
    }

    switch (direction) {
        case DIRECTION_NORTH:
            vehicle->x = INTERSECTION_X + LANE_WIDTH / 2 - vehicle->rect.w / 2;
            vehicle->y = WINDOW_HEIGHT + 10;
            break;
        case DIRECTION_SOUTH:
            vehicle->x = INTERSECTION_X - LANE_WIDTH / 2 - vehicle->rect.w / 2;
            vehicle->y = -40;
            break;
        case DIRECTION_EAST:
            vehicle->x = -40;
            vehicle->y = INTERSECTION_Y + LANE_WIDTH / 2 - vehicle->rect.h / 2;
            break;
        case DIRECTION_WEST:
            vehicle->x = WINDOW_WIDTH + 10;
            vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2 - vehicle->rect.h / 2;
            break;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    // Add to queue
    enqueue(&laneQueues[direction], *vehicle);

    return vehicle;
}

// Check if vehicle should stop for vehicle ahead IN SAME QUEUE
bool shouldStopForVehicleInQueue(Vehicle *vehicle, Direction lane) {
    float criticalDistance = 100.0f;
    
    Node *current = laneQueues[lane].front;
    while (current != NULL) {
        Vehicle *other = &current->vehicle;
        
        if (!other->active || other == vehicle) {
            current = current->next;
            continue;
        }
        
        float distance = 0;
        bool ahead = false;
        
        switch (lane) {
            case DIRECTION_NORTH:
                distance = vehicle->y - other->y;
                ahead = (other->y < vehicle->y) && (distance > 0);
                break;
            case DIRECTION_SOUTH:
                distance = other->y - vehicle->y;
                ahead = (other->y > vehicle->y) && (distance > 0);
                break;
            case DIRECTION_EAST:
                distance = other->x - vehicle->x;
                ahead = (other->x > vehicle->x) && (distance > 0);
                break;
            case DIRECTION_WEST:
                distance = vehicle->x - other->x;
                ahead = (other->x < vehicle->x) && (distance > 0);
                break;
        }
        
        if (ahead && distance < criticalDistance) {
            return true;
        }
        
        current = current->next;
    }
    
    return false;
}

void updateVehicle(Vehicle *vehicle, TrafficLight *lights) {
    if (!vehicle->active) return;

    float stopLine = 0;
    bool shouldStopLight = false;

    switch (vehicle->direction) {
        case DIRECTION_NORTH:
            stopLine = INTERSECTION_Y + LANE_WIDTH;
            shouldStopLight = (vehicle->y > stopLine - 80 && vehicle->y < stopLine + 10 && 
                             lights[DIRECTION_NORTH].state == RED);
            break;
        case DIRECTION_SOUTH:
            stopLine = INTERSECTION_Y - LANE_WIDTH;
            shouldStopLight = (vehicle->y < stopLine + 80 && vehicle->y > stopLine - 10 && 
                             lights[DIRECTION_SOUTH].state == RED);
            break;
        case DIRECTION_EAST:
            stopLine = INTERSECTION_X - LANE_WIDTH;
            shouldStopLight = (vehicle->x < stopLine + 80 && vehicle->x > stopLine - 10 && 
                             lights[DIRECTION_EAST].state == RED);
            break;
        case DIRECTION_WEST:
            stopLine = INTERSECTION_X + LANE_WIDTH;
            shouldStopLight = (vehicle->x > stopLine - 80 && vehicle->x < stopLine + 10 && 
                             lights[DIRECTION_WEST].state == RED);
            break;
    }

    // Check for vehicles ahead in same queue
    bool shouldStopVehicle = shouldStopForVehicleInQueue(vehicle, vehicle->direction);
    bool shouldStop = shouldStopLight || shouldStopVehicle;

    if (shouldStop) {
        vehicle->speed = 0;
        vehicle->state = STATE_STOPPED;
    } else if (vehicle->state == STATE_STOPPED) {
        vehicle->state = STATE_MOVING;
        vehicle->speed = 0.5f;
    } else if (vehicle->speed < 2.0f) {
        vehicle->speed += 0.1f;
        if (vehicle->speed > 2.0f) vehicle->speed = 2.0f;
    }

    if (vehicle->speed > 0) {
        switch (vehicle->direction) {
            case DIRECTION_NORTH: vehicle->y -= vehicle->speed; break;
            case DIRECTION_SOUTH: vehicle->y += vehicle->speed; break;
            case DIRECTION_EAST: vehicle->x += vehicle->speed; break;
            case DIRECTION_WEST: vehicle->x -= vehicle->speed; break;
        }
    }

    if (vehicle->y < -50 || vehicle->y > WINDOW_HEIGHT + 50 ||
        vehicle->x < -50 || vehicle->x > WINDOW_WIDTH + 50) {
        vehicle->active = false;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;
}

void renderRoads(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);

    SDL_Rect verticalRoad = {INTERSECTION_X - LANE_WIDTH, 0, LANE_WIDTH * 2, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &verticalRoad);

    SDL_Rect horizontalRoad = {0, INTERSECTION_Y - LANE_WIDTH, WINDOW_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &horizontalRoad);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < WINDOW_HEIGHT; i += 40) {
        SDL_Rect dash = {INTERSECTION_X - 2, i, 4, 20};
        SDL_RenderFillRect(renderer, &dash);
    }

    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        SDL_Rect dash = {i, INTERSECTION_Y - 2, 20, 4};
        SDL_RenderFillRect(renderer, &dash);
    }
}

void renderSimulation(SDL_Renderer *renderer, TrafficLight *lights, Statistics *stats) {
    // Beige background
    SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255);
    SDL_RenderClear(renderer);

    renderRoads(renderer);

    // Draw traffic lights
    for (int i = 0; i < 4; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect border = lights[i].position;
        border.x -= 2; border.y -= 2;
        border.w += 4; border.h += 4;
        SDL_RenderFillRect(renderer, &border);
        
        if (lights[i].state == RED) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        }
        SDL_RenderFillRect(renderer, &lights[i].position);
    }

    // Draw vehicles from all queues
    int totalRendered = 0;
    for (int lane = 0; lane < 4; lane++) {
        Node *current = laneQueues[lane].front;
        while (current != NULL) {
            Vehicle *v = &current->vehicle;
            if (v->active) {
                SDL_Color color = VEHICLE_COLORS[v->colorIndex];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                
                // Debug: print first vehicle position
                if (totalRendered == 0) {
                    printf("Rendering vehicle at (%d, %d) size (%d, %d)\n", 
                           v->rect.x, v->rect.y, v->rect.w, v->rect.h);
                }
                
                SDL_RenderFillRect(renderer, &v->rect);
                totalRendered++;
            }
            current = current->next;
        }
    }
    
    if (totalRendered > 0) {
        printf("Total vehicles rendered: %d\n", totalRendered);
    }

    SDL_RenderPresent(renderer);
}

// Queue functions
void initQueue(Queue *q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue *q, Vehicle vehicle) {
    Node *newNode = (Node *)malloc(sizeof(Node));
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
        Vehicle empty = {0};
        return empty;
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

void removeFromQueue(Queue *q, Vehicle *vehicle) {
    // Not needed in this simple implementation
}