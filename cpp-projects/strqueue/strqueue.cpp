#include "strqueue.h"
#include <unordered_map>
#include <deque>
#include <cassert>

namespace cxx {

#ifdef NDEBUG
const bool log = false;
#else
const bool log = true;
#endif

// Getter dla mapy przechowującej kolejne kolejki napisów z id jako klucz.
static std::unordered_map <unsigned long, std::deque<std::string> >& getQueues() {
    static std::unordered_map<unsigned long, std::deque <std::string> > queues;
    return queues;
}

// Getter dla statycznej zmiennej globalnej id.
static unsigned long& getId() {
    static unsigned long id = 0;
    return id;
}

// Funckja pomocnicza sprawdzająca czy w 'queues' znajduje się kolejka o danym id.
static bool exists(unsigned long id) {
    return getQueues().find(id) != getQueues().end();
}

unsigned long strqueue_new() {
    if constexpr (log) {
        std::cerr << "strqueue_new()\n";
    }

    std::deque <std::string> q;
    getQueues()[getId()] = q;

    if constexpr (log) {
        // Zapobiegamy asercji wyjściu 'Id' kolejki poza zakres zmiennej.
        assert(getId() + 1 > getId() && "ID overflow detected in strqueue_new!");
        std::cerr << "strqueue_new returns " << getId() << "\n";
    }
    return getId()++;
}

void strqueue_delete(unsigned long id) {
    if constexpr (log) {
        std::cerr << "strqueue_delete(" << id << ")\n";
    }

    if (!exists(id)) {
        if constexpr (log) {
            std::cerr << "strqueue_delete: queue " << id << " does not exist\n";
        }
        return;
    }
    else {
        getQueues().erase(id);

        if constexpr (log) {
            std::cerr << "strqueue_delete done\n";
        }
    }
}

size_t strqueue_size(unsigned long id) {
    if constexpr (log) {
        std::cerr << "strqueue_size(" << id << ")\n";
    }

    if (exists(id)) {
        size_t size = getQueues()[id].size();

        if constexpr (log) {
            std::cerr << "strqueue_size returns " << size << "\n";
        }
        return size;
    }

    if constexpr (log) {
        std::cerr << "strqueue_size: queue " << id << " does not exist\n";
        std::cerr << "strqueue_size returns 0\n";
    }
    return 0;
}

const char* strqueue_get_at(unsigned long id, size_t position) {
    if constexpr (log) {
        std::cerr << "strqueue_get_at(" << id << ", " << position << ")\n";
    }

    if (exists(id)) {
        if (position < getQueues()[id].size()) {    // Konwersja typu string na const char*.
            const char* result = getQueues()[id][position].c_str();

            if constexpr (log) {
                std::cerr << "strqueue_get_at returns \"" << result << "\"\n";
            }
            return result;
        }
        else if constexpr (log) {
            std::cerr << "strqueue_get_at: queue " << id << " does not contain string at position " << position << "\n";
            std::cerr << "strqueue_get_at returns NULL\n";
        }
    }
    else if constexpr (log) {
        std::cerr << "strqueue_get_at: queue " << id << " does not exist\n";
        std::cerr << "strqueue_get_at returns NULL\n";
    }
    return NULL;
}

void strqueue_insert_at(unsigned long id, size_t position, const char* str) {
    if constexpr (log) {
        std::cerr << "strqueue_insert_at(" << id << ", " << position
        << ", " << (str ? "\"" + std::string(str) + "\"" : "NULL") << ")\n";
    }

    // Przypadek gdy dostajemy pusty napis.
    if (str == NULL) {
        if constexpr (log) {
            std::cerr << "strqueue_insert_at failed\n";
        }
        return;
    }

    if (!exists(id)) {
        if constexpr (log) {
            std::cerr << "strqueue_insert_at: queue " << id << " does not exist\n";
        }
        return;
    }

    // Tworzymy kopię napisu z wejecia, zgodnie z poleceniem zadania.
    std::string copy = std::string(str);

    if (position >= getQueues()[id].size()) {
        getQueues()[id].push_back(copy);
    }
    else {
        getQueues()[id].insert(getQueues()[id].begin() + position, copy);
    }

    if constexpr (log) {
        std::cerr << "strqueue_insert_at done\n";
    }
}

void strqueue_remove_at(unsigned long id, size_t position) {
    if constexpr (log) {
        std::cerr << "strqueue_remove_at(" << id << ", " << position << ")\n";
    }

    if (!exists(id)) {
        if constexpr (log) {
            std::cerr << "strqueue_remove_at: queue " << id << " does not exist\n";
        }
        return;
    }

    // Przypadek gdy 'position' jest poza zakresem kolejki 'id'.
    if (position >= getQueues()[id].size()) {
        if constexpr (log) {
            std::cerr << "strqueue_remove_at: queue " << id << " does not contain string at position " << position << "\n";
        }
        return;
    } 

    getQueues()[id].erase(getQueues()[id].begin() + position);

    if constexpr (log) {
        std::cerr << "strqueue_remove_at done\n";
    }
}

void strqueue_clear(unsigned long id) {
    if constexpr (log) {
        std::cerr << "strqueue_clear(" << id << ")\n";
    }

    if (!exists(id)) {
        if constexpr (log) {
            std::cerr << "strqueue_clear: queue " << id << " does not exist\n";
        }
        return;
    }
    else {
        getQueues()[id].clear();
        if constexpr (log) {
            std::cerr << "strqueue_clear done\n";
        }
    }
}

int strqueue_comp(unsigned long id1, unsigned long id2) {
    if constexpr (log) {
        std::cerr << "strqueue_comp(" << id1 << ", " << id2 << ")\n";
    }

    //Ustawiamy referencje do kolejek o ID 'id1', 'id2' jeżeli istnieją, wpp. do pustych kolejek.
    const auto& queue1 = exists(id1) ? getQueues()[id1] : std::deque <std::string>();
    const auto& queue2 = exists(id2) ? getQueues()[id2] : std::deque <std::string>();

    if (!exists(id1)) {
        if constexpr (log) {
            std::cerr << "strqueue_comp: queue " << id1 << " does not exist\n";
        }
    }

    if (!exists(id2)) {
        if constexpr (log) {
            std::cerr << "strqueue_comp: queue " << id2 << " does not exist\n";
        }
    }

    int result = 0;
    //Porównywanie kolejek leksykograficznie za pomocą referencji.
    if (queue1 < queue2) result = -1;
    else if (queue1 > queue2) result = 1;

    if constexpr (log) {
        std::cerr << "strqueue_comp returns " << result << "\n";
    }
    return result;
}

}