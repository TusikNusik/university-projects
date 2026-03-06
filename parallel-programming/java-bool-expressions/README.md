# Concurrent Boolean Circuit Solver

A high-performance, multithreaded Java engine for evaluating complex boolean circuits. 

This project provides a highly concurrent `CircuitSolver` capable of processing massive, tree-like boolean expressions. It leverages **lazy evaluation** to skip unnecessary computations (e.g., short-circuiting an `OR` gate if one input is already `true`) and evaluates independent sub-expressions entirely in parallel.



## Key Features

* **High Concurrency:** Evaluates individual nodes of the boolean expression tree concurrently, maximizing CPU core utilization.
* **Lazy Evaluation:** Intelligently halts execution of sibling nodes if the parent's value is already determined (e.g., in `AND`, `OR`, `IF` nodes), saving computational resources.
* **Lock-Free/Low-Contention Design:** Built from the ground up using core Java concurrency primitives for granular thread control, purposefully avoiding high-level abstractions like `CompletableFuture`.
* **Graceful Interruption:** Supports immediate halting of the solver (`stop()`), which safely propagates `InterruptedException` to pending tasks without leaking threads or resources.
* **Thread-Safe Querying:** Multiple clients can submit circuits (`solve()`) and query results (`getValue()`) simultaneously without blocking the main submission thread.

## Supported Node Types

The solver processes Abstract Syntax Trees (AST) composed of the following logic gates:

* **Basic Logic:** `AND`, `OR`, `NOT`
* **Conditionals:** `IF(condition, ifTrue, ifFalse)`
* **Threshold Logic:** * `GTx (Greater Than):` Returns `true` if at least `x + 1` child nodes evaluate to `true`.
  * `LTx (Less Than):` Returns `true` if at most `x - 1` child nodes evaluate to `true`.
* **Leaves:** Base `LEAF` nodes containing raw boolean values (which may simulate long-running I/O or database fetches).

## Architecture

The project is structured around three main interfaces:

1. **`Circuit` / `CircuitNode`:** The data structure representing the directed acyclic graph (DAG) or tree of the boolean expression.
2. **`CircuitSolver`:** The core execution engine. Submitting a circuit via `solve(Circuit c)` immediately returns a future-like handle.
3. **`CircuitValue`:** A handle to the asynchronous result. Calling `getValue()` blocks only the requesting thread until the specific circuit's evaluation is complete.
