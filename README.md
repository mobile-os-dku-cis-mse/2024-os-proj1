# CPU Scheduling Simulation

This project simulates a basic CPU scheduling system using the **Round-Robin Scheduling** algorithm. The program creates multiple child processes from a parent process and simulates the execution of CPU-burst and I/O-burst cycles for each child. The parent process acts as the scheduler, handling process scheduling and I/O management using Inter-Process Communication (IPC).

---

## Features

1. **Round-Robin Scheduling**:
   - The parent process assigns CPU time slices to child processes using a time quantum.
   - The current process's CPU-burst is decremented with each time slice.
   - Scheduling operates on a periodic timer, implemented with `setitimer`.

2. **I/O Requests**:
   - Child processes request I/O operations after completing a CPU-burst.
   - The parent moves the process to a wait queue and simulates I/O by decrementing the I/O-burst over time.

3. **Process Management**:
   - Maintains a **run queue** for processes in a ready state.
   - Maintains a **wait queue** for processes performing I/O.
   - Moves processes between queues dynamically.

4. **Logging**:
   - Scheduling operations are logged to `schedule_dump.txt` in the following format:
     ```
     At time t, process pid gets CPU time, remaining CPU burst x
     Run Queue: [pids]
     Wait Queue: [pids]
     ```

5. **Signal Handling**:
   - The parent process uses `SIGALRM` to trigger periodic scheduling events.
   - A global pointer to the `Scheduler` struct ensures safe access to shared state in the signal handler.

---

## File Structure

```plaintext
.
├── scheduler.h         # Header file for shared structures and function prototypes
├── scheduler.c         # Implementation of scheduling logic
├── child.h             # Header file for child process functionality
├── child.c             # Implementation of child process logic
├── main.c              # Entry point for the parent process
├── schedule_dump.txt   # Log file for scheduling operations
├── Makefile            # Build automation script (optional, if implemented)
└── README.md           # Project documentation
```

---

## How It Works

### Parent Process

- **Initialization**:
  - Creates 10 child processes using `fork()`.
  - Configures periodic scheduling using `setitimer` and `SIGALRM`.

- **Scheduling**:
  - Handles periodic timer ticks and assigns CPU time slices to processes in the **run queue**.
  - Decrements I/O-burst values for processes in the **wait queue**.
  - Moves processes back to the **run queue** once I/O is complete.

- **Logging**:
  - Logs all scheduling events and the state of the run/wait queues to `schedule_dump.txt`.

### Child Process

- **Execution**:
  - Simulates a dynamic workload of alternating CPU-burst and I/O-burst.
  - After completing a CPU-burst, sends an I/O request to the parent using `msgsnd`.
  - Waits for messages from the parent to simulate progress during its CPU time slice.

---

## How to Build

Ensure you have GCC or a similar C compiler installed.

### Compile the Program

```bash
make
```

### Run the Program

```bash
./scheduler
```

### Output

- **schedule_dump.txt**: Contains logs of scheduling operations.

---

## Key Components

### Structures

- **`Scheduler`**:
  Encapsulates the state of the scheduling system, including:
  - Process details (`struct Process`)
  - Run and wait queues
  - Current process and time tick
  - File descriptor for logging

- **`my_msgbuf`**:
  Used for IPC between the parent and child processes.

### Inter-Process Communication (IPC)

- **Message Queue**:
  - Parent sends time slices to child processes using `msgsnd`.
  - Child processes send I/O requests back to the parent.

### Signal Handling

- **`SIGALRM`**:
  - Triggers scheduling events at regular intervals.

---

## Project Requirements

1. **CPU Scheduling**:
   - Implements Round-Robin scheduling with configurable time quantum.

2. **I/O Involvement**:
   - Simulates I/O requests and processes an I/O-burst using a wait queue.

3. **Logging**:
   - Outputs scheduling operations in a structured format.

4. **Memory Safety**:
   - Avoids using unnecessary global variables by encapsulating state in a `Scheduler` struct.

---

## Troubleshooting

### Segmentation Fault (SIGSEGV)
If a segmentation fault occurs:
- Ensure the `Scheduler` pointer is initialized and properly assigned to the global `global_scheduler`.
- Check that all processes and queues are correctly managed.

### Message Queue Issues
- Ensure the message queue is created using `msgget` and cleaned up with `msgctl`.
