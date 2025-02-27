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

---

## **Activity 1 - Thread-Based Client Management Simulation**

### **Objective:**
The goal of this assignment is to develop a multithreaded program that simulates a client management system using concepts of thread synchronization, process management, and inter-process communication in a Linux environment. The task involves implementing key functionalities such as client reception, queuing, and service handling while ensuring efficient coordination and prioritization.

### **Key Requirements:**

#### **1. Program Design:**
  - The program must use at least two threads:
    - **Reception Thread:** Responsible for creating client processes and adding them to a queue.
    - **Attendant Thread:** Handles the service of clients by removing them from the queue.
    - Additional threads can be implemented to manage queue control as needed.

#### **2. Client Characteristics:**
  - Clients are categorized into two priority levels:
    - **High Priority:** Maximum waiting patience is X/2 ms.
    - **Low Priority:** Maximum waiting patience is X ms.
    - Priority is assigned randomly with equal probability (50%).

#### **3. Inputs:**
  - **N:** Number of clients to be generated (0 indicates infinite clients).
  - **X:** Maximum patience time for low-priority clients.
  
#### **4. Outputs:**
  - **Satisfaction Rate:** Proportion of satisfied clients (served within their patience time).
  - **Total Execution Time:** Time taken to serve all clients.

#### **5. Additional Process - Analyst:**
  - Reads client IDs (PIDs) from a file written by the attendant.
  - Prints up to the first 10 IDs each time it is "awakened" and clears them from the file.

#### **6. Key Functionalities:**
  - Thread synchronization using pthreads.
  - Client handling with appropriate semaphore mechanisms.
  - Randomized priority assignment and queue insertion logic.
  - Dynamic stopping mechanism when **N=0** by pressing **'s'**.
  - Efficient management of shared resources like files and queues.

#### **7. Programming Environment:**
  - **Language:** C
  - **Platform:** Linux
  - **Restrictions:** No external libraries for parallelism or inter-process communication.

---

## **Activity 2 - Mini System with Custom Memory Management and File System**

### **Objective:**
Develop a mini operating system that includes a **custom file system** and **memory management system** with support for paging during sorting operations.

### **A. File System:**
- Must contain only the **root directory** and files with a **limited filename length**.
- Must be implemented inside a **1 GB file** created in the host system.

### **B. Supported Commands:**
1. **`create name size`**  
   - Creates a file named **"name"** (limited filename length allowed) containing a list of random **32-bit positive integers**.
   - The **"size"** parameter indicates the number of numbers.
   - The list can be stored in **binary format** or as a readable string (numbers separated by a delimiter like a comma or space).

2. **`delete name`**  
   - Deletes the file with the specified **"name"**.

3. **`list`**  
   - Displays the files in the directory.
   - Shows each file’s **size in bytes**.
   - At the end, it must also show the **total disk space** and **available space**.

4. **`sort name`**  
   - Sorts the list stored in the file **"name"**.
   - Any sorting algorithm may be used (library implementations are allowed).
   - Must display the **execution time in milliseconds** after sorting.

5. **`read name start end`**  
   - Displays a **sublist** of the file **"name"**, with the range given by **start** and **end** arguments.

6. **`concat name1 name2`**  
   - Concatenates the files **name1** and **name2**.
   - The concatenated file can either take the name of **name1** or have a predefined name.
   - The original files must be **deleted** after concatenation.

### **C. Memory Management:**
- A **Huge Page (2 MB)** must be allocated for sorting operations (on Linux, or an equivalent mechanism on another OS).
- No additional memory can be used for sorting; **paging must be implemented using the virtual disk**.
- The **paged memory system** should be implemented only for sorting and storing the number list.
- Other working memory (e.g., for variables, stack, or file system structure) may use regular system memory.
- The virtual disk for paging can be:
  - A **dedicated area** outside the file system.
  - A **special file** within the file system.
