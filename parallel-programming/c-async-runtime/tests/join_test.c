#include <assert.h>
#include <fcntl.h>
#include <stdint.h> // For uint64_t
#include <stdio.h> // For printf
#include <stdlib.h> // For exit
#include <string.h> // For memcmp
#include <sys/timerfd.h> // For timerfd
#include <unistd.h> // For pipe, read, write

#include "err.h"
#include "executor.h"
#include "future.h"
#include "future_combinators.h"
#include "future_examples.h"
#include "mio.h"
#include "utils.h"

typedef struct {
    Future base;
    int x;
    int64_t ret;
} WakerFuture;

FutureState waker_future_progress(Future* fut, Mio* mio, Waker waker) {
    WakerFuture *self = (WakerFuture*)fut;
    if (self->x > 0) {
        self->x--;
        waker_wake(&waker);
        return FUTURE_PENDING;
    } else {
        self->base.ok = (void*)self->ret;
        return FUTURE_COMPLETED;
    }
}

WakerFuture waker_future_create(int x, int64_t ret) {
    return (WakerFuture) {
        .base = future_create(waker_future_progress),
        .x = x,
        .ret = ret
    };
}

int main() {
    WakerFuture w1 = waker_future_create(10000, 42000);
    WakerFuture w2 = waker_future_create(10000, 69);
    JoinFuture j = future_join(&w1.base, &w2.base);

    Executor* executor = executor_create(2);
    executor_spawn(executor, &j.base);

    executor_run(executor);

    assert((int64_t)w1.base.ok == 42000);
    assert((int64_t)w2.base.ok == 69);
    assert((int64_t)j.result.fut1.ok == 42000);
    assert((int64_t)j.result.fut1.errcode == FUTURE_SUCCESS);
    assert((int64_t)j.result.fut2.ok == 69);
    assert((int64_t)j.result.fut1.errcode == FUTURE_SUCCESS);
    executor_destroy(executor);
    printf("mASLO");
}
