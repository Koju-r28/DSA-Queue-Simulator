#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include "traffic_simulator.h"

// Global flag for clean shutdown
volatile bool running = true;

// Signal handler for graceful shutdown (Ctrl+C)
void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("\n🛑 Stopping vehicle generator...\n");
        running = false;
    }
}

// FIXED: Added error checking and better formatting
void writeVehicleToFile(FILE *file, Vehicle *vehicle, int vehicleID) {
    if (!file || !vehicle) {
        fprintf(stderr, "❌ Error: NULL pointer in writeVehicleToFile\n");
        return;
    }

    // Write with timestamp and vehicle ID for better tracking
    Uint32 timestamp = SDL_GetTicks();
    
    fprintf(file, "VEHICLE_ID=%d TIME=%u X=%.2f Y=%.2f DIR=%d TYPE=%d TURN=%d STATE=%d SPEED=%.2f ACTIVE=%d\n", 
            vehicleID,
            timestamp,
            vehicle->x, 
            vehicle->y, 
            vehicle->direction, 
            vehicle->type, 
            vehicle->turnDirection, 
            vehicle->state, 
            vehicle->speed,
            vehicle->active);
}

// FIXED: Added function to create directory if it doesn't exist
bool ensureDirectoryExists(const char *path) {
#ifdef _WIN32
    return (_mkdir(path) == 0 || errno == EEXIST);
#else
    return (mkdir(path, 0755) == 0 || errno == EEXIST);
#endif
}

// FIXED: Added statistics tracking
typedef struct {
    int totalGenerated;
    int northCount;
    int southCount;
    int eastCount;
    int westCount;
    Uint32 startTime;
} GeneratorStats;

void printStats(GeneratorStats *stats) {
    Uint32 elapsed = (SDL_GetTicks() - stats->startTime) / 1000; // seconds
    printf("\n📊 Generator Statistics:\n");
    printf("   ⏱️  Running time: %u seconds\n", elapsed);
    printf("   🚗 Total vehicles: %d\n", stats->totalGenerated);
    printf("   ⬆️  North: %d (%.1f%%)\n", stats->northCount, 
           (stats->totalGenerated > 0) ? (stats->northCount * 100.0f / stats->totalGenerated) : 0);
    printf("   ⬇️  South: %d (%.1f%%)\n", stats->southCount, 
           (stats->totalGenerated > 0) ? (stats->southCount * 100.0f / stats->totalGenerated) : 0);
    printf("   ➡️  East: %d (%.1f%%)\n", stats->eastCount, 
           (stats->totalGenerated > 0) ? (stats->eastCount * 100.0f / stats->totalGenerated) : 0);
    printf("   ⬅️  West: %d (%.1f%%)\n", stats->westCount, 
           (stats->westCount > 0) ? (stats->westCount * 100.0f / stats->totalGenerated) : 0);
    
    if (elapsed > 0) {
        printf("   📈 Rate: %.2f vehicles/minute\n", (stats->totalGenerated * 60.0f / elapsed));
    }
    printf("\n");
}

// FIXED: Main function with proper SDL initialization and error handling
int main(int argc, char *argv[]) {
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Setup signal handler for graceful shutdown
    signal(SIGINT, signalHandler);
    
    // FIXED: Initialize SDL (required for SDL_GetTicks and SDL_Delay)
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "❌ SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    printf("✅ SDL Timer initialized\n");
    
    // FIXED: Ensure bin directory exists
    printf("📁 Checking bin directory...\n");
#ifdef _WIN32
    _mkdir("bin");
#else
    mkdir("bin", 0755);
#endif
    
    // FIXED: Open file with error checking
    FILE *file = fopen("bin/vehicles.txt", "w");
    if (!file) {
        perror("❌ Failed to open bin/vehicles.txt");
        SDL_Quit();
        return 1;
    }
    printf("✅ Opened bin/vehicles.txt for writing\n");
    
    // Write file header
    fprintf(file, "# Traffic Simulator Vehicle Data\n");
    fprintf(file, "# Generated at: %ld\n", time(NULL));
    fprintf(file, "# Format: VEHICLE_ID TIME X Y DIR TYPE TURN STATE SPEED ACTIVE\n");
    fprintf(file, "# ============================================================\n");
    fflush(file);
    
    // Initialize statistics
    GeneratorStats stats = {0};
    stats.startTime = SDL_GetTicks();
    
    printf("\n🚦 Vehicle Generator Started\n");
    printf("Press Ctrl+C to stop gracefully\n");
    printf("Generating vehicles every 2 seconds...\n\n");
    
    int vehicleID = 1;
    Uint32 lastPrintTime = SDL_GetTicks();
    
    // FIXED: Better loop with exit condition
    while (running) {
        // Generate a new vehicle with random direction
        Direction spawnDirection = (Direction)(rand() % 4);
        Vehicle *newVehicle = createVehicle(spawnDirection);
        
        // FIXED: Check if vehicle creation succeeded
        if (!newVehicle) {
            fprintf(stderr, "⚠️  Warning: Failed to create vehicle %d\n", vehicleID);
            SDL_Delay(100); // Brief delay before retry
            continue;
        }
        
        // Write the vehicle data to the file
        writeVehicleToFile(file, newVehicle, vehicleID);
        fflush(file); // Ensure data is written immediately
        
        // Update statistics
        stats.totalGenerated++;
        switch (spawnDirection) {
            case DIRECTION_NORTH: stats.northCount++; break;
            case DIRECTION_SOUTH: stats.southCount++; break;
            case DIRECTION_EAST:  stats.eastCount++;  break;
            case DIRECTION_WEST:  stats.westCount++;  break;
        }
        
        // Print progress (every 5 seconds)
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastPrintTime >= 5000) {
            printStats(&stats);
            lastPrintTime = currentTime;
        } else {
            // Simple progress indicator
            const char *dirSymbol[] = {"⬆️ ", "⬇️ ", "➡️ ", "⬅️ "};
            printf("✅ Vehicle #%d generated: %s Direction=%d Turn=%d\n", 
                   vehicleID, 
                   dirSymbol[spawnDirection],
                   spawnDirection,
                   newVehicle->turnDirection);
        }
        
        // Free the vehicle memory
        free(newVehicle);
        vehicleID++;
        
        // FIXED: Check running flag during delay (allow early exit)
        for (int i = 0; i < 20 && running; i++) {
            SDL_Delay(100); // Total 2 seconds (20 * 100ms)
        }
    }
    
    // Cleanup
    printf("\n🧹 Cleaning up...\n");
    
    // Write final statistics to file
    fprintf(file, "\n# ============================================================\n");
    fprintf(file, "# Generation completed\n");
    fprintf(file, "# Total vehicles generated: %d\n", stats.totalGenerated);
    fprintf(file, "# North: %d, South: %d, East: %d, West: %d\n", 
            stats.northCount, stats.southCount, stats.eastCount, stats.westCount);
    
    fclose(file);
    printf("✅ Closed vehicles.txt\n");
    
    SDL_Quit();
    printf("✅ SDL shutdown complete\n");
    
    // Print final statistics
    printStats(&stats);
    
    printf("👋 Vehicle generator stopped successfully\n");
    return 0;
}

// FIXED: For Windows compatibility, add SDL_main if needed
#ifdef _WIN32
#undef main
int SDL_main(int argc, char *argv[]) {
    return main(argc, argv);
}
#endif