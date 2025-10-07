export module event;

import <cstdint>;
import <ostream>;

export struct event {
    uint64_t timestamp; // Który z impulsów z obecnego A wywołał.
    uint64_t counter_id;

    bool operator<(const event& other) const noexcept {
        if (timestamp != other.timestamp) return timestamp < other.timestamp;
        else return counter_id < other.counter_id;
    }
};

export std::ostream& operator<<(std::ostream& os, const event& ev) {
    os << "E " << ev.counter_id << " " << ev.timestamp;
    return os;
}