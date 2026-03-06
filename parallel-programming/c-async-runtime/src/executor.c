#include "executor.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "future.h"
#include "mio.h"
#include "waker.h"
#include "err.h"

/**
 * @brief Structure to represent the current-thread executor.
    queue: stores PENDING futures,
    storage: distinguishes wakers that belong to futures that couldn't progress.
 */
struct Executor {
    // TODO: add your needed fields
    Future** queue;
    size_t start, end, size, toCompute, it, pending;
    Mio* mio;
    Waker* storage;
};

Executor* executor_create(size_t max_queue_size) { 
    Executor* new_executor = malloc(sizeof(Executor));
    if(new_executor == NULL)
        fatal("malloc");

    new_executor->mio = mio_create(new_executor);
    new_executor->queue = malloc(sizeof(Future*) * max_queue_size);
    new_executor->storage = calloc(max_queue_size, sizeof(Waker));
    if(new_executor->queue == NULL || new_executor->storage == NULL)
        fatal("malloc");

    new_executor->it = 0;
    new_executor->start = 0;
    new_executor->end = 0;
    new_executor->toCompute = 0;
    new_executor->pending = 0;
    new_executor->size = max_queue_size;

    return new_executor;
}

void waker_wake(Waker* waker) {
    if(waker == NULL || waker->executor == NULL || waker->future == NULL) 
        return;

    executor_spawn(waker->executor, waker->future);
}

void executor_spawn(Executor* executor, Future* fut) {
    fut->is_active = true;
    executor->toCompute++;
    executor->queue[executor->end] = fut;
    executor->end = (executor->end + 1) % executor->size;    
}

void executor_run(Executor* executor) { 

    // Executor runs until it has taks on queue or tasks that can't progress.
    while(executor->toCompute != 0 || executor->pending != 0) {
        if(executor->start == executor->end) {
            mio_poll(executor->mio);
        }
        else {
            Future* akt = executor->queue[executor->start];
            Waker waker;
            waker.future = akt;
            waker.executor = executor;
            FutureState state = akt->progress(akt, executor->mio, waker);
            executor->start = (executor->start + 1) % executor->size;
            executor->toCompute--;
            if(state != FUTURE_PENDING) {
                for(int i = 0; i < executor->size; i++) {
                    if(executor->storage[i].future == akt) {
                        executor->storage[i].future = NULL;
                        executor->pending--;
                    }
                }
                akt->is_active = false;
            }
            else {
                bool alreadyPending = false;
                for(int i = 0; i < executor->size; i++) {
                    if(executor->storage[i].future == akt)
                        alreadyPending = true;
                       
                }
                if(!alreadyPending) {
                    executor->pending++;
                    for(int i = 0; i < executor->size; i++) {
                        if(executor->storage[i].future == NULL) {
                            executor->storage[i].future = akt;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void executor_destroy(Executor* executor) {
    mio_destroy(executor->mio);
    free(executor->queue);
    free(executor->storage);
    free(executor);
}
