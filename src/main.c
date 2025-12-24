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
    TrafficLight lights[4];
    initializeTrafficLights(lights);
    Statistics stats = {
        .vehiclesPassed = 0,
        .totalVehicles = 0,
        .vehiclesPerMinute = 0,
        .startTime = SDL_GetTicks()
    };
    for (int i = 0; i < 4; i++) {
        initQueue(&laneQueues[i]);
    }

    printf("Traffic Simulation Started - Queue Based with LEFT TURNS\n");
    printf("GREEN BACKGROUND | 4 Queues: North, South, East, West\n");
    printf("15%% vehicles will turn LEFT, 85%% go STRAIGHT\n\n");

    while (running) {
        handleEvents(&running);

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastVehicleSpawn >= SPAWN_INTERVAL) {
            Direction spawnDirection = (Direction)(rand() % 4);
            Vehicle *newVehicle = createVehicle(spawnDirection);
            if (newVehicle) {
                const char* dirNames[] = {"NORTH", "SOUTH", "EAST", "WEST"};
                const char* turnNames[] = {"STRAIGHT", "LEFT", "RIGHT"};
                printf("=== SPAWNED: Dir=%s, Turn=%s, Color=%d, Queue size=%d ===\n", 
                       dirNames[spawnDirection], 
                       turnNames[newVehicle->turnDirection],
                       newVehicle->colorIndex,
                       laneQueues[spawnDirection].size);
                
                stats.totalVehicles++;
                free(newVehicle);
            }
            
            lastVehicleSpawn = currentTime;
        }
        
        static Uint32 lastDebug = 0;
        if (currentTime - lastDebug >= 3000) {
            printf("\n[STATUS] Queue sizes: N=%d, S=%d, E=%d, W=%d | Total passed: %d\n",
                   laneQueues[0].size, laneQueues[1].size, 
                   laneQueues[2].size, laneQueues[3].size,
                   stats.vehiclesPassed);
            lastDebug = currentTime;
        }
        for (int lane = 0; lane < 4; lane++) {
            Node *current = laneQueues[lane].front;
            Node *prev = NULL;
            
            while (current != NULL) {
                Vehicle *v = &current->vehicle;
                
                if (v->active) {
                    
                    updateVehicle(v, lights);
                    
                    if (!v->active) {
                        printf("Vehicle removed from lane %d, Queue size now: %d\n", 
                               lane, laneQueues[lane].size - 1);
                        stats.vehiclesPassed++;
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
        updateTrafficLights(lights);
        float minutes = (SDL_GetTicks() - stats.startTime) / 60000.0f;
        if (minutes > 0) {
            stats.vehiclesPerMinute = stats.vehiclesPassed / minutes;
        }
        renderSimulation(renderer, lights, &stats);

        SDL_Delay(16); 
    }

    cleanupSDL(window, renderer);
    return 0;
}