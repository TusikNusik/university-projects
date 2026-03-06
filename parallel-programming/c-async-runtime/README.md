# C-Async-Runtime 

A lightweight, single-threaded asynchronous executor for C, heavily inspired by modern asynchronous runtimes like Rust's Tokio. 

This project provides a cooperative multitasking environment that allows handling multiple concurrent I/O-bound tasks (such as network sockets or file descriptors) without the context-switching overhead of traditional multi-threading. By utilizing `epoll` under the hood, the executor ensures the application only uses CPU cycles when tasks are genuinely ready to make progress.



## Features

* **Zero-Thread Concurrency:** All tasks and the event loop run on a single thread, utilizing cooperative yielding.
* **`epoll`-backed I/O Reactor:** Efficient, level-triggered event notification for non-blocking I/O operations.
* **Custom `Future` Abstraction:** A C-native implementation of coroutines/futures with state retention.
* **Future Combinators:** Built-in utilities for chaining and running futures concurrently (`Then`, `Join`, `Select`).
* **Memory Safe & Leak-Free:** Designed to be run safely with AddressSanitizer (ASAN) and Valgrind.

## Core Architecture

The runtime is divided into four main structural components:

* **Executor:** The core event loop (`executor_run`). It maintains a queue of ready-to-run tasks. If the queue is empty, it delegates to the I/O reactor to wait for system events.
* **Mio (Reactor):** The communication layer with the OS. It wraps the `epoll` system call, allowing tasks to register interest in `read`/`write` readiness on specific file descriptors and suspending the main thread until events occur.
* **Future:** Represents an asynchronous computation or coroutine. Because C lacks inheritance, futures are implemented using a base `Future` struct as the first member of a specific task struct. 
* **Waker:** A callback mechanism. When a task cannot make progress (e.g., waiting for data), it yields. Once the underlying resource is ready, the Waker notifies the Executor to re-queue the task.

### Task Lifecycle

A `Future` in this runtime transitions through the following states:

1. **PENDING (Queued):** The task is currently running or waiting in the executor's queue.
2. **PENDING (Waiting):** The task yielded because it is waiting on I/O. The `Waker` is held by the `Mio` reactor.
3. **COMPLETED:** The computation finished successfully.
4. **FAILURE:** The computation encountered an error or was aborted.

## Future Combinators

To manage complex asynchronous control flow, this runtime implements three essential future combinators:

* **`ThenFuture` (Sequential):** Chains two futures. It executes `fut1` to completion, and passes its result as the input argument to `fut2`.
* **`JoinFuture` (Concurrent AND):** Executes `fut1` and `fut2` concurrently. It completes only when *both* underlying futures have successfully completed.
* **`SelectFuture` (Concurrent OR):** Executes `fut1` and `fut2` concurrently. It completes as soon as *either* future finishes successfully, immediately cancelling and abandoning the remaining future.

