# SimpleScheduler - A Process Scheduler in C from Scratch

## Design Document for Operating Systems (OS) Assignment 3

---

# Group Members & Contribution

## 2024138 - Atharva Singh Velpula

### Contributions

* Scheduler loop
* Round Robin queue
* `display_info()` function
* `SIGINT` handler for Scheduler

---

## 2024343 - Mayank Yadav

### Contributions

* Shared memory and semaphore setup & cleanup
* Shell loop
* `SIGCHLD` handler for Shell
* `dummy_main.h`

---

## Joint Contribution

Both members contributed to:

* Error handling
* Bug fixing

---

# Implementation Details

* The codebase is well documented using:

  * Comments
  * Appropriate variable names
  * Meaningful function names

---

# `SimpleShell.c`

## Functionality

* Runs an infinite loop to continuously accept user input.
* Terminates explicitly when the `exit` command is entered.

---

## Shared Memory & Semaphore Initialization

### Shared Memory

* Initializes shared memory for maintaining the process table.

### Semaphores

* Initializes semaphores for:

  * Mutual exclusion
  * Safe concurrent access while reading/writing shared memory

---

## Scheduler Creation

* Creates a separate scheduler process before starting the shell loop.

---

## `SIGCHLD` Handler

### Purpose

* Updates the process table whenever a child process finishes execution.

---

## Process Submission

### Supported Command

```bash id="8j91ho"
submit <executable>
```

### Workflow

* Creates a child process for the executable.
* Submits the process to the scheduler for scheduling.

---

## Exit Handling

### On `exit` Command

* Sends a termination signal to the scheduler.
* Waits for the scheduler process to terminate.
* Prints required execution details.
* Cleans up resources and exits gracefully.

---

# `SimpleScheduler.c`

## Functionality

* Continuously reads the process table in an infinite scheduling loop.
* Picks `NCPU` processes for execution for `TSLICE` milliseconds.

---

## Scheduler Workflow

### At Each Iteration

1. Pop `NCPU` processes from the queue.
2. Signal selected processes to execute.
3. Update:

   * Time spent on CPU
   * Time spent off CPU
4. Sleep for `TSLICE` milliseconds.
5. Signal running processes to stop.

---

## Scheduling Policy

### Round Robin Scheduling

* Processes are scheduled in a Round Robin fashion.
* Ensures fair CPU allocation among submitted processes.

---

## Termination Logic

### Case 1: Exit Signal + No Remaining Processes

* Scheduler terminates immediately.

### Case 2: Exit Signal + Remaining Processes

* Scheduler continues executing remaining processes.
* Terminates only after all processes complete execution.

---

## `SIGINT` Handler

### Purpose

* Helps manage graceful termination of the scheduler.

---

## Shared Resource Initialization

Before beginning the scheduling loop:

* Shared memory is initialized.
* Semaphores are initialized.

---

# `dummy_main.h`

## Functionality

* Contains boilerplate code provided in the assignment.

---

## Modification Made

The following line was added:

```c id="1v1b1r"
raise(SIGSTOP);
```

---

## Purpose of `SIGSTOP`

* Ensures each process blocks immediately after `exec()` execution.
* Prevents processes from executing automatically.
* Makes the process wait for:

```c id="gqq7gq"
SIGCONT
```

from the scheduler before execution begins.

---

# Process Scheduling Workflow

```text id="9qqv3a"
User submits executable
          ↓
     SimpleShell
          ↓
 Creates child process
          ↓
 Process added to shared process table
          ↓
    SimpleScheduler
          ↓
 Selects NCPU processes
          ↓
 Sends SIGCONT
          ↓
 Processes execute for TSLICE
          ↓
 Sends stop signal
          ↓
 Round Robin continues
```

---

# Shared Memory Architecture

## Shared Components

The following data is stored in shared memory:

* Process table
* Scheduling information
* CPU timing statistics

---

## Synchronization

### Semaphores

Used to:

* Prevent race conditions
* Ensure safe concurrent access
* Synchronize Shell and Scheduler operations

---

# Signal Usage

| Signal    | Purpose                         |
| --------- | ------------------------------- |
| `SIGSTOP` | Pause newly created processes   |
| `SIGCONT` | Resume scheduled processes      |
| `SIGCHLD` | Detect child process completion |
| `SIGINT`  | Graceful scheduler termination  |

---

# Key System Calls & Functions Used

* `fork()`
* `exec()`
* `wait()`
* `signal()`
* `raise()`
* `kill()`
* `shm_open()`
* `mmap()`
* `sem_init()`
* `sem_wait()`
* `sem_post()`

---

# Core Features

* Round Robin process scheduling
* Configurable CPU time slicing
* Shared memory based process table
* Semaphore synchronization
* Multi-process coordination using signals
* Graceful scheduler termination
* Execution tracking and statistics

---

# Notes

* Processes do not begin execution immediately after `exec()`.
* Scheduler-controlled execution is achieved using:

  * `SIGSTOP`
  * `SIGCONT`
* Shared memory enables communication between:

  * Shell
  * Scheduler
* Semaphore synchronization ensures consistency of the shared process table.
* The scheduler guarantees completion of submitted processes before termination.
