#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "traffic_simulation.h"

void initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_Init(SDL_INIT_VIDEO);
    *window = SDL_CreateWindow("Traffic Simulation - Queue Based", SDL_WINDOWPOS_UNDEFINED, 
                               SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 
                               SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(*renderer, 255, 255, 255, 255);
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

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    bool running = true;
    Uint32 lastVehicleSpawn = 0;
    const Uint32 SPAWN_INTERVAL = 2000;

    srand(time(NULL));
    initializeSDL(&window, &renderer);

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

    // Initialize 4 queues (one per lane)
    for (int i = 0; i < 4; i++) {
        initQueue(&laneQueues[i]);
    }

    printf("Traffic Simulation Started - Queue Based\n");
    printf("4 Queues: North, South, East, West\n");
    
    int frameCount = 0;

    while (running) {
        handleEvents(&running);

        Uint32 currentTime = SDL_GetTicks();
        frameCount++;

        // Spawn new vehicle every 2 seconds
        if (currentTime - lastVehicleSpawn >= SPAWN_INTERVAL) {
            Direction spawnDirection = (Direction)(rand() % 4);
            
            // Create vehicle and add to appropriate queue
            Vehicle *newVehicle = createVehicle(spawnDirection);
            if (newVehicle) {
                printf("Frame %d: Vehicle spawned in lane %d (N=0,S=1,E=2,W=3), Queue size: %d\n", 
                       frameCount, spawnDirection, laneQueues[spawnDirection].size);
                
                stats.totalVehicles++;
                free(newVehicle);
            }
            
            lastVehicleSpawn = currentTime;
        }
        
        // Debug: Print queue sizes every 100 frames
        if (frameCount % 100 == 0) {
            printf("Queue sizes: N=%d, S=%d, E=%d, W=%d\n",
                   laneQueues[0].size, laneQueues[1].size, 
                   laneQueues[2].size, laneQueues[3].size);
        }

        // Update all vehicles in all queues
        for (int lane = 0; lane < 4; lane++) {
            Node *current = laneQueues[lane].front;
            Node *prev = NULL;
            
            while (current != NULL) {
                Vehicle *v = &current->vehicle;
                
                if (v->active) {
                    // Update vehicle position
                    updateVehicle(v, lights);
                    
                    // Check if vehicle left the screen
                    if (!v->active) {
                        printf("Frame %d: Vehicle removed from lane %d, Queue size now: %d\n", 
                               frameCount, lane, laneQueues[lane].size - 1);
                        stats.vehiclesPassed++;
                        
                        // Remove from queue
                        Node *toDelete = current;
                        if (prev == NULL) {
                            laneQueues[lane].front = current->next;
                        } else {
                            prev->next = current->next;
                        }
                        
                        if (current == laneQueues[lane].rear) {
                            laneQueues[lane].rear = prev;
                        }
                        
                        current = current->next;
                        free(toDelete);
                        laneQueues[lane].size--;
                        continue;
                    }
                }
                
                prev = current;
                current = current->next;
            }
        }

        // Update traffic lights
        updateTrafficLights(lights);

        // Update statistics
        float minutes = (SDL_GetTicks() - stats.startTime) / 60000.0f;
        if (minutes > 0) {
            stats.vehiclesPerMinute = stats.vehiclesPassed / minutes;
        }

        // Render everything (passing lights and stats only)
        renderSimulation(renderer, lights, &stats);

        SDL_Delay(16); // ~60 FPS
    }

    cleanupSDL(window, renderer);
    return 0;
}