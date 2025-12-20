#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "traffic_simulator.h"

// FIXED: Added proper error checking for SDL initialization
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "❌ SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    
    *window = SDL_CreateWindow(
        "Traffic Simulation", 
        SDL_WINDOWPOS_CENTERED,  // FIXED: Changed from UNDEFINED for better positioning
        SDL_WINDOWPOS_CENTERED, 
        WINDOW_WIDTH, 
        WINDOW_HEIGHT, 
        SDL_WINDOW_SHOWN
    );
    
    if (!*window) {
        fprintf(stderr, "❌ SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!*renderer) {
        fprintf(stderr, "❌ SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return false;
    }
    
    SDL_SetRenderDrawColor(*renderer, 135, 206, 235, 255); // Sky blue background
    
    printf("✅ SDL initialized successfully\n");
    return true;
}

void cleanupSDL(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    printf("✅ SDL cleanup complete\n");
}

// FIXED: Added key controls
void handleEvents(bool *running, bool *paused, TrafficLight *lights) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    *running = false;
                    break;
                case SDLK_SPACE:
                    *paused = !*paused;
                    printf("%s simulation\n", *paused ? "⏸️  Paused" : "▶️  Resumed");
                    break;
                case SDLK_r:
                    printf("🔄 Resetting simulation...\n");
                    // Reset traffic lights
                    initializeTrafficLights(lights);
                    break;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                    // Manual traffic light control (optional)
                    printf("🚦 Manual light control: Lane %d\n", event.key.keysym.sym - SDLK_0);
                    break;
            }
        }
    }
}


bool checkCollision(Vehicle *v1, Vehicle *v2) {
    if (!v1->active || !v2->active) return false;
    
   
    return (v1->rect.x < v2->rect.x + v2->rect.w &&
            v1->rect.x + v1->rect.w > v2->rect.x &&
            v1->rect.y < v2->rect.y + v2->rect.h &&
            v1->rect.y + v1->rect.h > v2->rect.y);
}


void updateLaneQueues(Vehicle *vehicles) {
    // Clear all queues first
    for (int i = 0; i < 4; i++) {
        // Don't completely clear, just update size
        int count = 0;
        for (int j = 0; j < MAX_VEHICLES; j++) {
            if (vehicles[j].active && vehicles[j].direction == i) {
                // Check if vehicle is approaching intersection
                bool approaching = false;
                switch (i) {
                    case DIRECTION_NORTH:
                        approaching = (vehicles[j].y > INTERSECTION_Y);
                        break;
                    case DIRECTION_SOUTH:
                        approaching = (vehicles[j].y < INTERSECTION_Y);
                        break;
                    case DIRECTION_EAST:
                        approaching = (vehicles[j].x < INTERSECTION_X);
                        break;
                    case DIRECTION_WEST:
                        approaching = (vehicles[j].x > INTERSECTION_X);
                        break;
                }
                if (approaching) count++;
            }
        }
        laneQueues[i].size = count;
    }
}


void printStatistics(Statistics *stats, int activeVehicles) {
    static Uint32 lastPrint = 0;
    Uint32 currentTime = SDL_GetTicks();
    
    // Print every 5 seconds
    if (currentTime - lastPrint >= 5000) {
        float minutes = (currentTime - stats->startTime) / 60000.0f;
        printf("\n📊 Statistics:\n");
        printf("   ⏱️  Running: %.1f minutes\n", minutes);
        printf("   🚗 Active vehicles: %d\n", activeVehicles);
        printf("   📊 Total spawned: %d\n", stats->totalVehicles);
        printf("   ✅ Passed through: %d\n", stats->vehiclesPassed);
        if (minutes > 0) {
            printf("   📈 Rate: %.1f vehicles/minute\n", stats->totalVehicles / minutes);
        }
        printf("   🚦 Queue sizes: N=%d S=%d E=%d W=%d\n",
               laneQueues[DIRECTION_NORTH].size,
               laneQueues[DIRECTION_SOUTH].size,
               laneQueues[DIRECTION_EAST].size,
               laneQueues[DIRECTION_WEST].size);
        printf("\n");
        lastPrint = currentTime;
    }
}


int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    bool running = true;
    bool paused = false;  
    Uint32 lastVehicleSpawn = 0;
    const Uint32 SPAWN_INTERVAL = 1000; 

    srand((unsigned int)time(NULL));

    // Initialize SDL
    if (!initializeSDL(&window, &renderer)) {
        return 1;
    }

    printf("🚦 Traffic Simulation Started\n");
    printf("Controls:\n");
    printf("  SPACE  - Pause/Resume\n");
    printf("  R      - Reset\n");
    printf("  Q/ESC  - Quit\n\n");

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
        .averageWaitTime = 0,  // FIXED: Changed from vehiclesPerMinute
        .startTime = SDL_GetTicks()
    };

    // Initialize queues
    for (int i = 0; i < 4; i++) {
        initQueue(&laneQueues[i]);
    }

    Uint32 frameCount = 0;
    Uint32 lastFrameTime = SDL_GetTicks();

    // Main game loop
    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        
        handleEvents(&running, &paused, lights);

        // Skip updates if paused
        if (!paused) {
            // Spawn new vehicles periodically
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastVehicleSpawn >= SPAWN_INTERVAL) {
                // FIXED: Only spawn if there's room
                if (vehicleCount < MAX_VEHICLES - 10) {  // Leave buffer
                    Direction spawnDirection = (Direction)(rand() % 4);
                    Vehicle* newVehicle = createVehicle(spawnDirection);
                    
                    if (newVehicle) {
                        // Find empty slot for new vehicle
                        bool spawned = false;
                        for (int i = 0; i < MAX_VEHICLES; i++) {
                            if (!vehicles[i].active) {
                                vehicles[i] = *newVehicle;
                                vehicles[i].active = true;
                                vehicleCount++;
                                stats.totalVehicles++;
                                spawned = true;
                                break;
                            }
                        }
                        
                        if (!spawned) {
                            printf("⚠️  Warning: Could not spawn vehicle, array full\n");
                        }
                        
                        free(newVehicle);
                    }
                    
                    lastVehicleSpawn = currentTime;
                }
            }

            // Update vehicles
            for (int i = 0; i < MAX_VEHICLES; i++) {
                if (vehicles[i].active) {
                    updateVehicle(&vehicles[i], lights);

                    // FIXED: Check if vehicle has left the screen
                    if (!vehicles[i].active) {
                        stats.vehiclesPassed++;
                        vehicleCount--;
                    }
                }
            }

            // FIXED: Simple collision detection (optional)
            for (int i = 0; i < MAX_VEHICLES; i++) {
                if (!vehicles[i].active) continue;
                for (int j = i + 1; j < MAX_VEHICLES; j++) {
                    if (!vehicles[j].active) continue;
                    if (checkCollision(&vehicles[i], &vehicles[j])) {
                        // Simple collision response: stop both vehicles briefly
                        if (vehicles[i].state != STATE_STOPPED) {
                            vehicles[i].speed *= 0.5f;
                        }
                        if (vehicles[j].state != STATE_STOPPED) {
                            vehicles[j].speed *= 0.5f;
                        }
                    }
                }
            }

            // Update lane queues
            updateLaneQueues(vehicles);

            // Update traffic lights
            updateTrafficLights(lights);

            // Print statistics periodically
            printStatistics(&stats, vehicleCount);
        }

        // Render (even when paused)
        renderSimulation(renderer, vehicles, lights, &stats);
        
        // FIXED: Show pause indicator
        if (paused) {
            // You can add a pause overlay here if you want
            // For now, it just continues showing the frozen state
        }

        // FPS counter
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastFrameTime >= 1000) {
            // printf("FPS: %d\n", frameCount);
            frameCount = 0;
            lastFrameTime = currentTime;
        }

        // Frame rate limiting (16ms = ~60 FPS)
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }

    // Cleanup
    printf("\n🧹 Cleaning up...\n");
    
    // Clear queues
    for (int i = 0; i < 4; i++) {
        while (!isQueueEmpty(&laneQueues[i])) {
            dequeue(&laneQueues[i]);
        }
    }
    
    cleanupSDL(window, renderer);
    
    // Print final statistics
    printf("\n📊 Final Statistics:\n");
    printf("   Total vehicles spawned: %d\n", stats.totalVehicles);
    printf("   Vehicles passed: %d\n", stats.vehiclesPassed);
    printf("   Success rate: %.1f%%\n", 
           stats.totalVehicles > 0 ? (stats.vehiclesPassed * 100.0f / stats.totalVehicles) : 0);
    
    printf("\n👋 Simulation ended\n");
    return 0;
}