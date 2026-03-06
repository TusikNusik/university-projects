package cp2024.solution;

import cp2024.circuit.*;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class ParallelCircuitValue implements CircuitValue {
    private Circuit task;
    BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();
    private boolean value, done;
    private Thread t;
    private SolveNode res;
    public ParallelCircuitValue(Circuit c) {
        task = c;
        done = false;
        res = new SolveNode(task.getRoot(), 0, q);
        t = new Thread(res);

    }
    public void stopThread() {
        if(!t.isAlive() || t.isInterrupted())
            return;
        t.interrupt();
    }
    @Override
    public boolean getValue() throws InterruptedException {
        if(done)
            return value;

        t.start();
        t.join();
        res.interruptOthers();
        done = true;
        value = res.value();
        return res.value();
    }
}
