# OS_UFC_2024.2
This repository contains projects, exercises, and materials developed during the Operating Systems course at Federal University of Ceará [2024.2].

The focus is on exploring fundamental concepts such as:
- Process and thread management
- Memory management
- File systems
- CPU scheduling
- Inter-process communication (IPC)
- Synchronization and deadlocks
  
The projects involve practical implementations using C and simulate scenarios of how an operating system works internally.

# **Activity 1 - Thread-Based Client Management Simulation**

## Objective:
The goal of this assignment is to develop a multithreaded program that simulates a client management system using concepts of thread synchronization, process management, and inter-process communication in a Linux environment. The task involves implementing key functionalities such as client reception, queuing, and service handling while ensuring efficient coordination and prioritization.

## Key Requirements:

### 1. Program Design:
  - The program must use at least two threads:
    - Reception Thread: Responsible for creating client processes and adding them to a queue.
    - Attendant Thread: Handles the service of clients by removing them from the queue.
    - Additional threads can be implemented to manage queue control as needed.

### 2. Client Characteristics:
  - Clients are categorized into two priority levels:
    - High Priority: Maximum waiting patience is X/2 ms.
    - Low Priority: Maximum waiting patience is X ms.
    - Priority is assigned randomly with equal probability (50%).

### 3. Inputs:
  - N: Number of clients to be generated (0 indicates infinite clients).
  - X: Maximum patience time for low-priority clients.
  
### 4. Outputs:
  - Satisfaction Rate: Proportion of satisfied clients (served within their patience time).
  - Total Execution Time: Time taken to serve all clients.

### 6. Additional Process - Analyst:
  - Reads client IDs (PIDs) from a file written by the attendant.
  - Prints up to the first 10 IDs each time it is "awakened" and clears them from the file.

### 7. Key Functionalities:
  - Thread synchronization using pthreads.
  - Client handling with appropriate semaphore mechanisms.
  - Randomized priority assignment and queue insertion logic.
  - Dynamic stopping mechanism when N=0 by pressing 's'.
  - Efficient management of shared resources like files and queues.

### 8. Programming Environment:
  - Language: C
  - Platform: Linux
  - Restrictions: No external libraries for parallelism or inter-process communication.
