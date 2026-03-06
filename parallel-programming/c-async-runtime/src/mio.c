#include "mio.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdbool.h>

#include "debug.h"
#include "executor.h"
#include "waker.h"
#include "err.h"

// Maximum number of events to handle per epoll_wait call.
#define MAX_EVENTS 64

// fdNode represents file descriptor and it hold Waker's that need to be waken when certain event happens at this fd.

typedef struct fdNode {
    int fd, mode;
    Waker* pollIN;
    Waker* pollOUT;
    size_t sOUT, sIN, itIN, itOUT;
    struct fdNode* next;
    struct fdNode* prev;
} fdNode;

// Mio holds list which contains added file descriptors and needed reasources. 
struct Mio {
    // TODO: add required fields
    Executor* executor;
    int epoll_fd;
    struct epoll_event pullEvents[MAX_EVENTS];
    fdNode* head;
};

fdNode* fdExists(Mio* mio, int fd) {
    fdNode* akt = mio->head;
    while(akt != NULL) {
        if(akt->fd == fd)
            return akt;

        akt = akt->next;
    }
    return NULL;
}

fdNode* fdAdd(Mio* mio, int fd) {
    fdNode* add = malloc(sizeof(fdNode));
    if(add == NULL)
        fatal("malloc");

    add->fd = fd;
    add->pollIN = malloc(sizeof(Waker) * 4);
    add->pollOUT = malloc(sizeof(Waker) * 4);
    if(add->pollIN == NULL || add->pollOUT == NULL)
        fatal("malloc");

    add->sOUT = 4;
    add->sIN = 4;
    add->itIN = 0;
    add->itOUT = 0;
    add->mode = 0;
    add->next = NULL;
    add->prev = NULL;

    fdNode* akt = mio->head;
    if(akt == NULL) {
        mio->head = add;
        return add;
    }

    while(akt->next != NULL) {
        akt = akt->next;
    }
    akt->next = add;
    add->prev = akt;

    return add;
}

void wakerAdd(fdNode* x, Waker waker, uint32_t event) {
    if(event == EPOLLIN) {
        if(x->itIN == x->sIN) {
            x->sIN *= 2;
            x->pollIN = realloc(x->pollIN, x->sIN * sizeof(Waker));
            if(x->pollIN == NULL)
                fatal("realloc");
        }
        x->pollIN[x->itIN] = waker;
        x->itIN++;
    }
    else {
        if(x->itOUT == x->sOUT) {
            x->sOUT *= 2;
            x->pollOUT = realloc(x->pollOUT, x->sOUT * sizeof(Waker));
            if(x->pollOUT == NULL)
                fatal("realloc");
        }
        x->pollIN[x->itOUT] = waker;
        x->itOUT++;
    }
}


// If on certain fd event occurs I wake all Waker's and "erase" the array.
void wakerWake(fdNode* x, uint32_t event) {
    if(event == EPOLLIN) {
        for(int i = 0; i < x->itIN; i++)
            waker_wake(&x->pollIN[i]);
        x->itIN = 0;
    }
    else {
        for(int i = 0; i < x->itOUT; i++)
            waker_wake(&x->pollOUT[i]);
        x->itOUT = 0;
        
        if(event != EPOLLOUT)   // event == EPOLLIN | EPOLLOUT
            wakerWake(x, EPOLLIN);
    }
}

Mio* mio_create(Executor* executor) {
    Mio* mio = malloc(sizeof(Mio));
    if(mio == NULL)
        fatal("malloc");

    ASSERT_SYS_OK(mio->epoll_fd = epoll_create1(0));
    mio->head = NULL;
    mio->executor = executor;

    return mio;
}

void mio_destroy(Mio* mio) {
    while(mio->head != NULL)
        mio_unregister(mio, mio->head->fd);

    ASSERT_SYS_OK(close(mio->epoll_fd));
    free(mio);
}

int mio_register(Mio* mio, int fd, uint32_t events, Waker waker)
{
    debug("Registering (in Mio = %p) fd = %d with", mio, fd);

    fdNode* temp = fdExists(mio, fd);

    if(temp == NULL) 
        temp = fdAdd(mio, fd);

    wakerAdd(temp, waker, events);

    bool doCTL = false;
    struct epoll_event add;
    add.events = events;

    // Events are both EPOLLIN and EPOLLOUT and we havent changed that yet.
    if(temp->mode != events && temp->mode != 0 && temp->mode != EPOLLIN + EPOLLOUT) {
        ASSERT_SYS_OK(epoll_ctl(mio->epoll_fd, EPOLL_CTL_DEL, fd, NULL));
        add.events = EPOLLIN | EPOLLOUT;
        doCTL = true;
        temp->mode = EPOLLIN + EPOLLOUT;
    }

    if(temp->mode == 0 || doCTL) {
        add.data.fd = fd;
        ASSERT_SYS_OK(epoll_ctl(mio->epoll_fd, EPOLL_CTL_ADD, fd, &add));
    }

    if(temp->mode == 0)
        temp->mode = events;

    return 0;
}

int mio_unregister(Mio* mio, int fd)
{
    debug("Unregistering (from Mio = %p) fd = %d\n", mio, fd);

    fdNode* temp = fdExists(mio, fd);

    if(temp == NULL)
        return -1;

    ASSERT_SYS_OK(epoll_ctl(mio->epoll_fd, EPOLL_CTL_DEL, fd, NULL));

    if(temp == mio->head)
        mio->head = temp->next;

    if(temp->prev == NULL && temp->next != NULL)
        temp->next->prev = NULL;
    if(temp->prev != NULL && temp->next == NULL)
        temp->prev->next = NULL;
    if(temp->prev != NULL && temp->next != NULL) {
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
    }

    free(temp->pollIN);
    free(temp->pollOUT);
    free(temp);
    return 0;
}

void mio_poll(Mio* mio)
{
    debug("Mio (%p) polling\n", mio);

    int event_count = epoll_wait(mio->epoll_fd, mio->pullEvents, MAX_EVENTS, 0);
	
	for(int i = 0; i < event_count; i++) {
		fdNode* temp = fdExists(mio, mio->pullEvents[i].data.fd);
        if(temp != NULL)
            wakerWake(temp, mio->pullEvents[i].events);
	}
}
