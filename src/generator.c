#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>  /
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define SLEEP(ms) usleep((ms) * 1000)
#endif

#include "traffic_simulation.h"

void writeVehicleToFile(FILE *file, Vehicle *vehicle) {
    fprintf(file, "%f %f %d %d %d %d %d %d\n",
            vehicle->x, vehicle->y,
            vehicle->direction,
            vehicle->type,
            vehicle->turnDirection,
            vehicle->state,
            vehicle->speed,
            vehicle->colorIndex);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    #ifdef _WIN32
        _mkdir("bin");  
    #else
        mkdir("bin", 0777);  // Unix function
    #endif

    FILE *file = fopen("bin/vehicles.txt", "w");
    if (!file) {
        perror("Failed to open vehicles.txt for writing");
        printf("Make sure the 'bin' directory exists in the current directory.\n");
        return 1;
    }

    printf("Vehicle Generator Started\n");
    printf("Writing vehicles to bin/vehicles.txt\n");
    printf("Press Ctrl+C to stop\n\n");

    int vehicleCounter = 0;
    const int MAX_CONCURRENT_VEHICLES = 15;  
    const int MIN_SPACING = 150;  

    Vehicle activeVehicles[MAX_CONCURRENT_VEHICLES] = {0};
    int activeCount = 0;

    while (1) {
      
        Direction spawnDirection = (Direction)(rand() % 4);
        Vehicle *newVehicle = createVehicle(spawnDirection);
        
        if (newVehicle) {
            
            bool canSpawn = true;
            for (int i = 0; i < activeCount; i++) {
                if (activeVehicles[i].direction == newVehicle->direction) {
                    
                    float distance = 0;
                    switch (newVehicle->direction) {
                        case DIRECTION_NORTH:
                        case DIRECTION_SOUTH:
                            distance = fabs(newVehicle->y - activeVehicles[i].y);
                            break;
                        case DIRECTION_EAST:
                        case DIRECTION_WEST:
                            distance = fabs(newVehicle->x - activeVehicles[i].x);
                            break;
                    }
                    
                    if (distance < MIN_SPACING) {
                        canSpawn = false;
                        break;
                    }
                }
            }
            
            if (canSpawn) {
                vehicleCounter++;
                printf("Generated Vehicle #%d - Direction: %d, Color: %d\n",
                       vehicleCounter, newVehicle->direction, newVehicle->colorIndex);

                
                if (activeCount < MAX_CONCURRENT_VEHICLES) {
                    activeVehicles[activeCount] = *newVehicle;
                    activeCount++;
                } else {
                   
                    for (int i = 0; i < activeCount - 1; i++) {
                        activeVehicles[i] = activeVehicles[i + 1];
                    }
                    activeVehicles[activeCount - 1] = *newVehicle;
                }

                
                fseek(file, 0, SEEK_SET);
                for (int i = 0; i < activeCount; i++) {
                    writeVehicleToFile(file, &activeVehicles[i]);
                }
                fflush(file);
            }

            free(newVehicle);
        }

        
        SLEEP(1500); 
    }

    fclose(file);
    return 0;
}