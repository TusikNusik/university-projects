export module counter_manager;

import <cstdint>;
export import <unordered_map>;
import <memory>;
import <vector>;
import <algorithm>;
import <iostream>;

import event;
import counter;

export class counter_manager {
    std::unordered_map<uint64_t, std::unique_ptr<counter>> counters;

public:
    counter_manager() = default;

    bool contains(uint64_t id) const {
        return counters.contains(id);
    }

    uint64_t get_counter_value(uint64_t id) const {
        return counters.at(id)->get_value();
    }

    void insert(uint64_t id, std::unique_ptr<counter>&& new_counter) {
        counters.try_emplace(id, std::move(new_counter));
    }

    template <typename CounterType, typename ...Types>
    bool try_add_index(uint64_t id, Types&&... args) {
        if (counters.contains(id)) return false;
        insert(id, std::make_unique<CounterType>(std::forward<Types>(args)...));
        return true;
    }

    void erase(uint64_t id) {
        counters.erase(id);
    }

    void advance(uint64_t amount) {
        std::vector<event> events;
        for (const auto &[id, counter_pointer] : counters) {
            for (uint64_t impulse_timestamp : counter_pointer->receive_pulses(amount)) {
                events.emplace_back(impulse_timestamp, id);
            }
        }

        std::sort(events.begin(), events.end());

        for (const event& ev : events) {
            std::cout << ev << "\n";
        }
    }
};