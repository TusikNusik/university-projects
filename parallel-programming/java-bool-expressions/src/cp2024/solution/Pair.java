package cp2024.solution;

class Pair<K,L> {
    private K lewy;
    private L prawy;
    public Pair(K l, L p) {
        lewy = l;
        prawy = p;
    }
    public K getLewy() {
        return lewy;
    }
    public L getPrawy() {
        return prawy;
    }
}