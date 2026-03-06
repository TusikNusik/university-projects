package cp2024.solution;

import cp2024.circuit.*;

import java.util.ArrayList;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
public class SolveNode implements Runnable {

    private int childNumber;
    private CircuitNode circuitNode;
    private BlockingQueue<Pair<Integer, Boolean>> parentQueue;
    private ArrayList <Pair<Thread,Integer>> children;
    private ArrayList <Integer> finishedChildren;

    private boolean nodeValue;
    public SolveNode(CircuitNode c, int ch, BlockingQueue<Pair<Integer, Boolean>> z) {
        circuitNode = c;
        children = new ArrayList<>();
        finishedChildren = new ArrayList<>();
        childNumber = ch;
        parentQueue = z;
    }
    // Chooses method based on NodeType then submits result to his parent's queue.
    // While interrupted recursivly interrupt all its unfinished children.
    @Override
    public void run() {
        try {
            if (circuitNode.getType() == NodeType.LEAF) {
                nodeValue = ((LeafNode) circuitNode).getValue();
            } else {
                CircuitNode[] child_nodes = circuitNode.getArgs();

                nodeValue = switch (circuitNode.getType()) {
                    case IF -> solveIF(child_nodes);
                    case AND -> solveAND(child_nodes);
                    case OR -> solveOR(child_nodes);
                    case GT -> solveGT(child_nodes, ((ThresholdNode) circuitNode).getThreshold());
                    case LT -> solveLT(child_nodes, ((ThresholdNode) circuitNode).getThreshold());
                    case NOT -> solveNOT(child_nodes);
                    default -> throw new RuntimeException("Illegal type " + circuitNode.getType());
                };
            }
            parentQueue.add(new Pair<>(childNumber, nodeValue));

        }
        catch (InterruptedException e) {
            interruptOthers();
        }
    }
    public boolean value() {
        return nodeValue;
    }
    // Interrupts unfinished children.
    public void interruptOthers() {
        for(Pair<Thread,Integer> x: children) {
            if (!finishedChildren.contains(x.getPrawy())) {
                x.getLewy().interrupt();
            }
        }
    }
    // Preparing and creating threads for calculating the node.
    public void initThreads(CircuitNode[] args, Thread[] x, SolveNode[] y, BlockingQueue <Pair<Integer,Boolean>> q) throws InterruptedException{
        for(int i = 0; i < args.length; i++) {
            if(Thread.interrupted()) throw new InterruptedException();

            y[i] = new SolveNode(args[i], i, q);
            x[i] = new Thread(y[i]);
            x[i].start();
            children.add(new Pair<>(x[i], i));
        }
    }
    private boolean solveNOT(CircuitNode[] args) throws InterruptedException {
        if(Thread.interrupted()) throw new InterruptedException();

        // Creating needed structures and starting thread manually.
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();
        SolveNode x = new SolveNode(args[0],0, q);
        Thread t = new Thread(x);
        t.start();

        if(Thread.interrupted()) throw new InterruptedException();
        boolean wyn = q.take().getPrawy();
        return !wyn;
    }

    private boolean solveLT(CircuitNode[] args, int threshold) throws InterruptedException {
        int correct = 0, iterations = 1;
        if(Thread.interrupted()) throw new InterruptedException();

        Thread[] threads = new Thread[args.length];
        SolveNode[] solveNodes = new SolveNode[args.length];
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();

        initThreads(args, threads, solveNodes, q);

        for(int i = 0; i < args.length; i++) {
            if(Thread.interrupted()) throw new InterruptedException();
            Pair<Integer,Boolean> para = q.take();
            // Adding to list of threads that will soon be completed / are competed.
            finishedChildren.add(para.getLewy());
            if(para.getPrawy())
                correct++;
            // We can end thread right here, lazy evaluation.
            if(correct >= threshold) {
                interruptOthers();
                return false;
            }
            if(args.length - iterations + correct < threshold) {
                interruptOthers();
                return true;
            }
        }
        if(correct >= threshold)
            return false;
        return true;
    }

    private boolean solveGT(CircuitNode[] args, int threshold) throws InterruptedException {
        int correct = 0, iterations = 1;
        if(Thread.interrupted()) throw new InterruptedException();

        Thread[] threads = new Thread[args.length];
        SolveNode[] solveNodes = new SolveNode[args.length];
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();

        initThreads(args, threads, solveNodes, q);

        for(int i = 0; i < args.length; i++) {
            if(Thread.interrupted()) throw new InterruptedException();

            Pair<Integer,Boolean> para = q.take();
            finishedChildren.add(para.getLewy());
            if(para.getPrawy())
                correct++;
            if(args.length - iterations + correct <= threshold) {
                interruptOthers();
                return false;
            }
            if(correct > threshold) {
                interruptOthers();
                return true;
            }
            iterations ++;
        }
        return true;
    }

    private boolean solveOR(CircuitNode[] args) throws InterruptedException {
        if(Thread.interrupted()) throw new InterruptedException();

        Thread[] threads = new Thread[args.length];
        SolveNode[] solveNodes = new SolveNode[args.length];
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();

        initThreads(args, threads, solveNodes, q);

        for(int i = 0; i < args.length; i++) {
            if(Thread.interrupted()) throw new InterruptedException();

            Pair<Integer,Boolean> para = q.take();
            finishedChildren.add(para.getLewy());
            // Lazy evaluation.
            if(para.getPrawy()) {
                interruptOthers();
                return true;
            }
        }
        return false;
    }

    private boolean solveAND(CircuitNode[] args) throws InterruptedException {
        if(Thread.interrupted()) throw new InterruptedException();

        Thread[] threads = new Thread[args.length];
        SolveNode[] solveNodes = new SolveNode[args.length];
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();

        initThreads(args, threads, solveNodes, q);

        for(int i = 0; i < args.length; i++) {
            if(Thread.interrupted()) throw new InterruptedException();

            Pair<Integer,Boolean> para = q.take();
            finishedChildren.add(para.getLewy());
            if(!para.getPrawy()) {
                interruptOthers();
                return false;
            }
        }
        return true;
    }

    private boolean solveIF(CircuitNode[] args) throws InterruptedException {
        if(Thread.interrupted()) throw new InterruptedException();
        BlockingQueue <Pair<Integer,Boolean>> q = new LinkedBlockingQueue<>();

        // Bonus arrays created to end soveIF as early as possible.
        boolean[] boolValues = new boolean[3];
        boolean[] checked = new boolean[3];
        boolean chooseLeft = false;

        Thread[] threads = new Thread[args.length];
        SolveNode[] solveNodes = new SolveNode[args.length];
        initThreads(args, threads, solveNodes, q);

        // Bunch of if statements cover a few possiblities when we can end solveIF early.
        // For example for (a,b,c) when we first get b result we can check if(a == true) and end without waitiing for c.

        for(int i = 0; i < 3; i++) {
            if(Thread.interrupted()) throw new InterruptedException();

            Pair<Integer,Boolean> para = q.take();

            if(para.getLewy() == 0) {
                checked[0] = true;
                finishedChildren.add(0);
                if(para.getPrawy()) {
                    chooseLeft = true;
                    if (checked[1]) {
                        interruptOthers();
                        return boolValues[1];
                    }
                }
                else {
                    if(checked[2]) {
                        interruptOthers();
                        return boolValues[2];
                    }
                }
            }
            else if(para.getLewy() == 1) {
                boolValues[1] = para.getPrawy();
                checked[1] = true;
                finishedChildren.add(1);
                if(checked[0]) {
                    if (chooseLeft) {
                        interruptOthers();
                        return para.getPrawy();
                    }
                }
                else {
                    if(checked[2] && boolValues[2] == boolValues[1]) {
                        interruptOthers();
                        return boolValues[1];
                    }
                }
            }
            else {
                boolValues[2] = para.getPrawy();
                checked[2] = true;
                finishedChildren.add(2);
                if(checked[0]) {
                    if (!chooseLeft) {
                        interruptOthers();
                        return para.getPrawy();
                    }
                }
                else {
                    if(checked[1] && boolValues[2] == boolValues[1]) {
                        interruptOthers();
                        return boolValues[2];
                    }
                }
            }
        }
        if(chooseLeft)
            return boolValues[1];
        return boolValues[2];

    }
}
