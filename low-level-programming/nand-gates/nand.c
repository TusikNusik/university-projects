#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include "nand.h"

struct n {  //struct reprezentujacy wierzcholek listy
    nand_t *ptr;
    struct n *next;
};

typedef struct n node;

struct nand {
    int deep;
    bool visited, pending;  // zmienne pomocnicze do funkcji nand_evaluate
    bool *exit;
    bool const **entrance; // tablica stalych wskaznikow do booli, trzymam podlaczone sygnaly
    node *out;  // wskaznik reprezentujacy HEAD listy trzymajacej podlaczone bramki
    nand_t **in; // tablica wskaznikow bramek do ktorych dane wejscie k jest podlaczone
    size_t r_in; // ilosc bramek wejsciowych
};

bool cycle = false; // zmienne ulatwiajace implementacje
bool allocation_failure = false;

// funkcja dodajaca nowy wierzecholek do konca listy 'x' ktory przechowuje wskaznik do bramki 'ad_tr'
void add(node *x, nand_t *ad_ptr) {
    if (x == NULL || ad_ptr == NULL)
        return;

    if (x->next == NULL) {
        node *n = (node*)malloc(sizeof(node));
        if(n == NULL) {
            allocation_failure = true;
            return;
        }
        n->next = NULL;
        n->ptr = ad_ptr;
        x->next = n;
    }
    else {
        add(x->next, ad_ptr); // iteruje sie na koniec listy
    }
}

// bramka 'del' jest usuwana wiec ustawiam wskazniki podlaczonej do niej bramki 'a' na NULL
void del_in(nand_t *a, nand_t *del) {
    if (a == NULL || del == NULL)
        return;

    for (size_t i = 0; i < a->r_in; i++) {
        if(a->in[i] == del) {  // znalazlem szukany indeks w tablicy
            a->in[i] = NULL;
            a->entrance[i] = NULL;
        }
    }
}

// funkcja usuwa node przechowujacy wskaznik do bramki 'del' z listy bramki 'x'
void exclude(nand_t *x, nand_t *del) {
    bool found = false;
    if (x == NULL || del == NULL)
        return;

    node *n = x->out;
    node *prev = n;

    if (n == NULL)
        return;

    while (n->next != NULL) {
        prev = n;
        n = n->next;

        if(n->ptr == del) { //jezeli znalazlo szukany wskaznik, przerywa petle
            found = true;
            break;
        }
    }

    if (n->next != NULL) // przepinam w odpowiedni sposob
        prev->next = n->next;
    else
        prev->next = NULL;

    if (found == true) //if controlny zapobiegajacy free(NULL);
        free(n);
}

// usuwa liste bramki 'g'
void rmv_list(node *x, nand_t *g) {
    if (x == NULL)
        return;

    rmv_list(x->next, g);

    if (x->ptr != NULL) {
        nand_t *a = x->ptr; // zapobiega wskaznikom do zwolnionej pamieci
        del_in(a, g);
    }

    free(x);
}

int max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

// sprawdza czy wszystkie wejscia bramki 'x' sa zapelnione sygnalami/bramkami
bool connected(nand_t *x) {
    if (x == NULL)
        return false;

    for (size_t i = 0; i < x->r_in; i++) {
        if(x->in[i] == NULL && x->entrance[i] == NULL)
            return false;
    }

    return true;
}
// funkcja ta ma dwie funkcje oblicza sygnal uzyskany rekurencyjnie oraz ustawia zmienne
// pomocnicze na ich odpowiednie wartosci
bool clean(nand_t *x) {
    if (x == NULL)
        return true;

    bool ans = false;  // ans domyslnie jest false

    for (size_t i = 0; i < x->r_in; i++) {
        if(x->in[i] != NULL) {
            if(clean(x->in[i]) == false)  // otrzymuje rekurencyjnie odpowiedni sygnal
                ans = true;
        }
        else if(x->entrance[i] != NULL) {
            if (*(x->entrance[i]) == false)
                ans = true;
        }
        else {
            ans = true;   //daje sygnal false jezeli na WSZYSTKICH jej wejsciach sa sygnaly true
        }
    }

    x->visited = false; // czysci po funkcji dfs/fnd
    x->pending = false;
    x->deep = 1;

    return ans;
}
// znajduje dlugosc najdluszej sciezki w drzewie o korzeniu 'x'
int fnd(nand_t *x) {
    int odp = 0;
    x->visited = true;

    for(size_t i = 0; i < x->r_in; i++) {
        if(x->in[i] != NULL && x->in[i]->r_in > 0) // rozpatruje bramki z niezerowa liczba wejsc
        {
            if(x->in[i]->visited == true)  // wartosc ta juz jest zapisana
                odp = max(odp, x->in[i]->deep);
            else
                odp = max(odp, fnd(x->in[i]));
        }
    }

    x->deep = odp + 1;
    return x->deep;
}
// znajduje ewentualny cykl w drzewie
void dfs(nand_t* x) {
    if(x == NULL)
        return;

    bool check = connected(x);    //sprawdza czy wszystkie wejscia 'x' sa zapelnione sygnalami/bramkami
    if(check == false)
        cycle = true;

    if(cycle == true)
        return;

    x->visited = true;
    x->pending = true;

    for(size_t i = 0; i < x->r_in; i++)
    {
        if(x->in[i] != NULL && x->in[i]->r_in > 0)
        {
            if(x->in[i]->pending == true) // istnieje krawedz idaca w gore drzewa czyli cykl
                cycle = true;
            else
                dfs(x->in[i]);
        }
    }

    x->pending = false; // wszystkie wejscia 'x' zostaly rozpatrzone
}

nand_t* nand_new(unsigned n) {
    nand_t *gate = (nand_t*)malloc(sizeof(nand_t));

    if (gate == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    gate->entrance = (bool const**)malloc(sizeof(bool const*) * n);

    if (gate->entrance == NULL) {
        free(gate);
        errno = ENOMEM;
        return NULL;
    }

    gate->in = (nand_t**)malloc(sizeof(nand_t*) * n);

    if (gate->in == NULL) {
        free(gate->entrance);
        free(gate);
        errno = ENOMEM;
        return NULL;
    }

    gate->out = (node*)malloc(sizeof(node));

    if (gate->out == NULL) {
        free(gate->in);
        free(gate->entrance);
        free(gate);
        errno = ENOMEM;
        return NULL;
    }
    // ustawiam wartosci startowe
    gate->exit = NULL;
    gate->out->ptr = NULL;
    gate->out->next = NULL;
    gate->visited = false;
    gate->pending = false;
    gate->deep = 1;
    gate->r_in = n;

    for (unsigned int i = 0; i < n; i++)
    {
        gate->in[i] = NULL;
        gate->entrance[i] = NULL;
    }

    return gate;
}

void nand_delete(nand_t *g) {
    if (g == NULL)
        return;

    for (size_t i = 0; i < g->r_in; i++) {
        if(g->in[i] != NULL)
            exclude(g->in[i], g);  // usuwa ten wierzcholek z listy 'out' bramki 'g->in[i]'
    }

    rmv_list(g->out, g);
    free(g->in);
    free(g->entrance);
    free(g);
}

int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k) {
    if (g_out == NULL || g_in == NULL || k >= g_in->r_in)  {
        errno = EINVAL;
        return -1;
    }

    if (g_in->in[k] != NULL)
        exclude(g_in->in[k], g_in);  // odlacza aktualnie podlaczona bramke

    add(g_out->out, g_in);

    if (allocation_failure == true) {
        errno = ENOMEM;
        allocation_failure = false;
        return -1;
    }

    g_in->in[k] = g_out;
    g_in->entrance[k] = g_out->exit;

    return 0;
}

int nand_connect_signal(bool const *s, nand_t *g, unsigned k) {
    if (s == NULL || g == NULL || k >= g->r_in) {
        errno = EINVAL;
        return -1;
    }

    if (g->in[k] != NULL) {
        exclude(g->in[k], g);  // odlacza aktualnie podlaczona bramke
        g->in[k] = NULL;
    }

    g->entrance[k] = s;

    return 0;
}

ssize_t nand_evaluate(nand_t **g, bool *s, size_t m)  {
    if (g == NULL || s == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (m <= 0) {
        errno = EINVAL;
        return -1;
    }

    ssize_t odp = 0;
    size_t wyz = 0; // zmienna sprawdzajaca czy g sklada sie z samych NULL

    for (size_t i = 0; i < m; i++) {
        if(g[m - i - 1] == NULL || wyz > 0) {
           wyz++;
        }
        else if(g[i]->r_in > 0) {
            dfs(g[i]);

            if(cycle == true)  // wystepuje cykl
            {
                cycle = false;
                errno = ECANCELED;
                return -1;
            }

            s[i] = clean(g[i]);
            odp = max(odp, fnd(g[i]));
            s[i] = clean(g[i]);
        }
        else if(g[i] -> r_in == 0) {
            s[i] = false; //pusta bramka daje false
        }
    }

    if (wyz == m) { //zwroci blad jezeli tablica g zawiera same NULLE;
        errno = EINVAL;
        return -1;
    }

    return odp;
}

ssize_t nand_fan_out(nand_t const *g) {
    if (g == NULL) {
        errno = EINVAL;
        return -1;
    }

    ssize_t x = 0;
    node *n = g->out;

    if (n == NULL)
        return -1;

    while (n->next != NULL) { // iteruje sie do konca listy zliczajac ilosc jej elementow
        x++;
        n = n->next;
    }

    return x;
}

void* nand_input(nand_t const *g, unsigned k) {
    if (g == NULL || k >= g->r_in) {
        errno = EINVAL;
        return NULL;
    }

    if (g->in[k] != NULL)
        return g->in[k];

    if (g->entrance[k] != NULL)
        return (bool *)g->entrance[k];

    errno = 0;
    return NULL;
}

nand_t* nand_output(nand_t const *g, ssize_t k) {
    if (g == NULL) {
        errno = EINVAL;
        return NULL;
    }

    node *x = g->out;

    if (x->next == NULL)
        return NULL;

    x = x->next;  //jezeli istnieje node poza HEAD chce od niego zaczac
    ssize_t it = 0;

    while (x->next != NULL && it < k) {
        x = x->next; // iteruje dopoki jeden z warunkow nie bedzie juz spelniony
        it++;
    }

    return x->ptr;
}

