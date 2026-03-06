#include "future_combinators.h"
#include <stdlib.h>

#include "future.h"
#include "waker.h"
#include "mio.h"

FutureState progress_then(Future* base, Mio* mio, Waker waker) {

    ThenFuture* temp = (ThenFuture*)base;

    if(!temp->fut1_completed) {

        FutureState state = temp->fut1->progress(temp->fut1, mio, waker);

        if(state == FUTURE_COMPLETED) {
            temp->fut1_completed = true;
            temp->fut2->arg = temp->fut1->ok;
        }
        else {
            if(state == FUTURE_FAILURE) 
                temp->base.errcode = THEN_FUTURE_ERR_FUT1_FAILED;
            return state;
        }
           
    }
    if(temp->fut1_completed) {

        FutureState state = temp->fut2->progress(temp->fut2, mio, waker);
        if (state == FUTURE_FAILURE) {
            temp->base.errcode = THEN_FUTURE_ERR_FUT2_FAILED;
            return FUTURE_FAILURE;
        }
        if(state == FUTURE_PENDING)
            return FUTURE_PENDING;

        temp->base.ok = temp->fut2->ok;
    }

    return FUTURE_COMPLETED;
}

FutureState progress_join(Future* base, Mio* mio, Waker waker) {
    JoinFuture* temp = (JoinFuture*)base;

    if(temp->fut1_completed == FUTURE_PENDING) {
        FutureState state = temp->fut1->progress(temp->fut1, mio, waker);
        if(state == FUTURE_PENDING)
            return FUTURE_PENDING;
        temp->fut1_completed = state;
    }
    if(temp->fut2_completed == FUTURE_PENDING) {
        FutureState state = temp->fut2->progress(temp->fut2, mio, waker);
        if(state == FUTURE_PENDING)
            return FUTURE_PENDING;
        temp->fut2_completed = state;
    }

    temp->result.fut1.ok = temp->fut1->ok;
    temp->result.fut2.ok = temp->fut2->ok;
    temp->result.fut1.errcode = temp->fut1->errcode;
    temp->result.fut2.errcode = temp->fut2->errcode;

    if(temp->fut1_completed != FUTURE_FAILURE && temp->fut2_completed != FUTURE_FAILURE) {
        temp->base.errcode = 0;
        temp->base.ok = temp->fut2->ok;
        return FUTURE_COMPLETED;
    }
    if(temp->fut1_completed != FUTURE_FAILURE && temp->fut2_completed == FUTURE_FAILURE) 
        temp->base.errcode = JOIN_FUTURE_ERR_FUT2_FAILED;
    if(temp->fut1_completed == FUTURE_FAILURE && temp->fut2_completed != FUTURE_FAILURE) 
        temp->base.errcode = JOIN_FUTURE_ERR_FUT1_FAILED;
    if(temp->fut1_completed == FUTURE_FAILURE && temp->fut2_completed == FUTURE_FAILURE) 
        temp->base.errcode = JOIN_FUTURE_ERR_BOTH_FUTS_FAILED;
    return FUTURE_FAILURE;

}

FutureState progress_select(Future* base, Mio* mio, Waker waker) {

    SelectFuture* temp = (SelectFuture*)base;
    
    if(temp->which_completed != SELECT_FAILED_FUT1) {
        FutureState state = temp->fut1->progress(temp->fut1, mio, waker);

        if(state == FUTURE_COMPLETED) {
            temp->which_completed = SELECT_COMPLETED_FUT1;
            temp->base.ok = temp->fut1->ok;
            return FUTURE_COMPLETED;
        }

        if(state == FUTURE_FAILURE && temp->which_completed == SELECT_FAILED_FUT2) {
            temp->which_completed = SELECT_FAILED_BOTH;
            return FUTURE_FAILURE;
        }

        if(state == FUTURE_FAILURE)
            temp->which_completed = SELECT_FAILED_FUT1;
    }

    if(temp->which_completed != SELECT_FAILED_FUT2) {
        FutureState state = temp->fut1->progress(temp->fut2, mio, waker);

        if(state == FUTURE_COMPLETED) {
            temp->which_completed = SELECT_COMPLETED_FUT2;
            temp->base.ok = temp->fut2->ok;
            return FUTURE_COMPLETED;
        }

        if(state == FUTURE_FAILURE && temp->which_completed == SELECT_FAILED_FUT1) {
            temp->which_completed = SELECT_FAILED_BOTH;
            return FUTURE_FAILURE;
        }

        if(state == FUTURE_FAILURE) 
            temp->which_completed = SELECT_FAILED_FUT2;
    }
    return FUTURE_PENDING;
}


ThenFuture future_then(Future* fut1, Future* fut2)
{
    return (ThenFuture) {
        .base = future_create(progress_then),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = false,
    };
}

JoinFuture future_join(Future* fut1, Future* fut2)
{
    return (JoinFuture) {
        .base = future_create(progress_join),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = FUTURE_PENDING,
        .fut2_completed = FUTURE_PENDING,
    };
}

SelectFuture future_select(Future* fut1, Future* fut2)
{
    return (SelectFuture) {
        .base = future_create(progress_select),
        .fut1 = fut1,
        .fut2 = fut2,
        .which_completed = SELECT_COMPLETED_NONE,
    };
}
