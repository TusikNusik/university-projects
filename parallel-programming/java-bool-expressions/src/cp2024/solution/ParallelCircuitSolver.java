package cp2024.solution;

import cp2024.circuit.CircuitNode;
import cp2024.circuit.CircuitSolver;
import cp2024.circuit.CircuitValue;
import cp2024.circuit.Circuit;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class ParallelCircuitSolver implements CircuitSolver {
    // HashMap which prevents from creating new CircutValue for the same node.
    private HashMap<Circuit, ParallelCircuitValue> tasks = new HashMap<>();
    @Override
    public CircuitValue solve(Circuit c) {
        if (tasks.containsKey(c))
            return tasks.get(c);

        ParallelCircuitValue p = new ParallelCircuitValue(c);
        tasks.put(c, p);
        return p;
    }

    @Override
    public void stop() {
        for (Circuit c: tasks.keySet())
            tasks.get(c).stopThread();
    }
}
