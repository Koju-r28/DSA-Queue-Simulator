 🚦 DSA Queue Simulator
## Four-Way Junction Traffic Management System
This project implements a queue-based traffic management simulation for a four-way junction, developed as part of the Data Structure and Algorithms (COMP202) course.

![Traffic Simulation](https://github.com/Koju-r28/DSA-Queue-Simulator/blob/main/Traffic_simulation.gif)

## 📋 Project Overview
The system manages vehicle flow using queue data structures at a four-way intersection. Traffic control is achieved through dynamic signal adjustment based on queue occupancy and vehicle turn intentions.

### Key Features
- 🚗 **Queue-based Traffic Management**: Each lane maintains its own FIFO queue for systematic vehicle processing
- 🔄 **Left Turn Functionality**: 15% of vehicles execute smooth left turns at the intersection
- 🚨 **Intelligent Traffic Signals**: Left-turning vehicles can proceed on red; straight vehicles must stop
- 🎨 **Real-time Visualization**: SDL2-powered graphics with pastel-colored vehicles on green background
- 📊 **Collision Avoidance**: Maintains 100-pixel minimum separation between vehicles
- 🖥️ **Performance Optimized**: Smooth 60 FPS rendering

## 📊 Data Structures

| Structure | Implementation | Purpose |
|-----------|----------------|---------|
| Queue | Array-based | Storing vehicles in each lane |
| Priority Queue | Modified queue with dynamic priority | Managing high-priority lanes |
## 🏗️ System Architecture

### Traffic Management Rules
1. **Signal Control**: Traffic lights alternate every 5 seconds between North-South and East-West
2. **Left Turn Priority**: Vehicles intending to turn left can proceed through red lights
3. **Straight Vehicle Protocol**: Vehicles going straight must stop at red signals
4. **Queue Processing**: Each direction maintains FIFO order for fair vehicle service
5. **Safety Distance**: 100-pixel minimum gap enforced between consecutive vehicles

### Core Components
- **main.c**: Application entry point, SDL initialization, main event loop
- **traffic_simulation.c**: Queue operations, vehicle logic, collision detection, rendering
- **traffic_simulation.h**: Data structures, function declarations, constants
## ⚙️ Algorithm Design

### Main Processing Flow
```
1. Initialize SDL and create 4 lane queues
2. Spawn vehicle every 2 seconds → Random direction, 15% turn left
3. For each lane queue:
   - Check collision with vehicle ahead
   - Check traffic light (left turn can go on red)
   - Execute turn if at intersection center
   - Move vehicle forward
   - Remove if off-screen
4. Update traffic lights (5-second cycle)
5. Render roads, lights, vehicles
6. Repeat at 60 FPS
```

### Time Complexity Analysis
| Operation | Complexity | Description |
|-----------|-----------|-------------|
| Enqueue/Dequeue | O(1) | Queue operations |
| Vehicle Update | O(n) | Update all vehicles |
| Collision Check | O(n) | Check same lane |
| **Overall** | **O(n)** | n = active vehicles |

## 🚀 Installation and Setup

### Prerequisites
- C++ Compiler (GCC or MinGW)
- SDL2 library
- Git (optional)

### Installation Steps
**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-SDL2
```

**Windows (Manual):**
- Download SDL2 from [https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)

---

### Building the Project

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/dsa-queue-simulator.git
cd dsa-queue-simulator

# Compile the generator and Compile the simulator
	g++ -o bin/generator src/generator.c src/traffic_simulation.c -lSDL2 -Iinclude -Llib -lmingw32 -lSDL2main -lSDL2

	g++ -Iinclude -Llib -o bin/main.exe src/main.c src/traffic_simulation.c -lmingw32 -lSDL2main -lSDL2

#Run the program
./bin/main.exe
