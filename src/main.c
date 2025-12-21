
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "traffic_simulation.h"

void initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_Init(SDL_INIT_VIDEO);
    *window = SDL_CreateWindow("Traffic Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(*renderer, 255, 255, 255, 255); // Set background color to white
}

void cleanupSDL(SDL_Window *window, SDL_Renderer *renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void handleEvents(bool *running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false;
        }
    }
}

Vehicle readVehicleFromFile(FILE *file) {
    Vehicle vehicle = {0};
    if (fscanf(file, "%f %f %d %d %d %d %d", 
           &vehicle.x, &vehicle.y, 
           (int*)&vehicle.direction, 
           (int*)&vehicle.type, 
           (int*)&vehicle.turnDirection, 
           (int*)&vehicle.state, 
           &vehicle.speed) == 7) {
        vehicle.active = true;
        
        // Set dimensions based on direction
        if (vehicle.direction == DIRECTION_NORTH || vehicle.direction == DIRECTION_SOUTH) {
            vehicle.rect.w = 20;
            vehicle.rect.h = 30;
        } else {
            vehicle.rect.w = 30;
            vehicle.rect.h = 20;
        }
        
        vehicle.rect.x = (int)vehicle.x;
        vehicle.rect.y = (int)vehicle.y;
    }
    return vehicle;
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    bool running = true;
    Uint32 lastVehicleSpawn = 0;
    const Uint32 SPAWN_INTERVAL = 500; // Spawn a vehicle every 500ms

    srand(time(NULL));

    initializeSDL(&window, &renderer);

    // Initialize vehicles
    Vehicle vehicles[MAX_VEHICLES] = {0};
    int vehicleCount = 0;

    // Initialize traffic lights
    TrafficLight lights[4];
    initializeTrafficLights(lights);

    // Initialize statistics
    Statistics stats = {
        .vehiclesPassed = 0,
        .totalVehicles = 0,
        .vehiclesPerMinute = 0,
        .startTime = SDL_GetTicks()
    };

    // Initialize queues
    for (int i = 0; i < 4; i++) {
        initQueue(&laneQueues[i]);
    }

    while (running) {
        handleEvents(&running);

        // Spawn new vehicles periodically
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastVehicleSpawn >= SPAWN_INTERVAL && vehicleCount < MAX_VEHICLES) {
            Direction spawnDirection = (Direction)(rand() % 4);
            Vehicle* newVehicle = createVehicle(spawnDirection);
            
            // Find empty slot for new vehicle
            for (int i = 0; i < MAX_VEHICLES; i++) {
                if (!vehicles[i].active) {
                    vehicles[i] = *newVehicle;
                    vehicles[i].active = true;
                    vehicleCount++;
                    stats.totalVehicles++;
                    break;
                }
            }
            
            free(newVehicle);
            lastVehicleSpawn = currentTime;
        }

        // Update vehicles
        for (int i = 0; i < MAX_VEHICLES; i++) {
            if (vehicles[i].active) {
                updateVehicle(&vehicles[i], lights);

                // Check if vehicle has passed through intersection
                if (!vehicles[i].active) {
                    stats.vehiclesPassed++;
                    vehicleCount--;
                }
            }
        }

        // Update traffic lights
        updateTrafficLights(lights);

        // Update statistics
        float minutes = (SDL_GetTicks() - stats.startTime) / 60000.0f;
        if (minutes > 0) {
            stats.vehiclesPerMinute = stats.vehiclesPassed / minutes;
        }

        renderSimulation(renderer, vehicles, lights, &stats);

        SDL_Delay(16); // Cap at ~60 FPS
    }

    cleanupSDL(window, renderer);
    return 0;
}