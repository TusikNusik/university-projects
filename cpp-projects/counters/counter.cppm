export module counter;

import <cstdint>;
import <vector>;
import <utility>;

import event;

export class counter {
    uint64_t ignore;
    const uint64_t divisor;
protected:
    uint64_t value;

public:
    counter(uint64_t divisor) : ignore(0), divisor(divisor), value(0){};

    // Zwiększa wartość licznika o pulses, albo do osiągnięcia zdarzenia.
    // Zwraca ile pulsów zużyło oraz czy pojawiło się zdarzenie.
    virtual std::pair<uint64_t, bool> advance(uint64_t pulses) noexcept = 0;

    uint64_t get_value() const noexcept { return value; }

    std::vector<uint64_t> receive_pulses(uint64_t pulses) {
        uint64_t pulse_distance = 0;

        std::vector<uint64_t> result;
        while (pulses) {
            uint64_t to_ignore = std::min(ignore, pulses);
            ignore -= to_ignore;
            pulses -= to_ignore;
            pulse_distance += to_ignore;

            uint64_t not_ignored_pulses;
            if (divisor != UINT64_MAX) {
                not_ignored_pulses = (pulses / (divisor + 1)) + (pulses % (divisor + 1) != 0 ? 1 : 0); // ceil(pulses/(divisor+1))
            } else {
                not_ignored_pulses = pulses > 0 ? 1 : 0;
            }

            auto [used_real_pulses, generated_event] = advance(not_ignored_pulses);

            uint64_t used_ignored_pulses = used_real_pulses > 0 ? (used_real_pulses - 1) * divisor : 0;

            pulses -= used_ignored_pulses + used_real_pulses;
            pulse_distance += used_ignored_pulses + used_real_pulses;

            if (generated_event) {
                result.push_back(pulse_distance);
            }

            if (used_real_pulses > 0) {
                ignore = divisor;
            } else break;
        }
        return result;
    }

    virtual ~counter() noexcept = default;
};

export class modulo_counter : public counter {
    const uint64_t modulus;
    bool count_to_max;

public:
    modulo_counter(uint64_t divisor, uint64_t modulus)
        : counter(divisor), modulus(modulus + 1), count_to_max(modulus == UINT64_MAX){};


    std::pair<uint64_t, bool> advance(uint64_t pulses) noexcept override {
        if (count_to_max) {
            const uint64_t distance_to_max = UINT64_MAX - value;
            if (distance_to_max < pulses) {
                value = 0;
                return {distance_to_max + 1, true};
            } else {
                value += pulses;
                return {pulses, false};
            }
        } else {
            const uint64_t distance_to_event = modulus - (value % modulus);
            const uint64_t used_pulses = std::min(distance_to_event, pulses);
            value = (value + used_pulses) % modulus;
            return {used_pulses, used_pulses == distance_to_event};
        }
    }
};

export class geometric_counter : public counter {
    uint64_t treshold;
    static const uint64_t maxTreshold = 999999999999;

public:
    geometric_counter(uint64_t divisor) : counter(divisor), treshold(9){};

    std::pair<uint64_t, bool> advance(uint64_t pulses) noexcept override {
        const uint64_t distance_to_event = treshold - value + 1;
        const uint64_t used_pulses = std::min(distance_to_event, pulses);

        value += used_pulses;
        if (used_pulses == distance_to_event) {
            value = 0;
            next_treshold();
        }
        return {used_pulses, used_pulses == distance_to_event};
    }

private:
    void next_treshold() noexcept {
        if (treshold == maxTreshold) treshold = 9;
        else treshold = treshold * 10 + 9;
    }
};

export class fibonacci_counter : public counter {
    static const uint64_t maxTreshold = UINT64_MAX;
    uint64_t fib1, fib2;

public:
    fibonacci_counter(uint64_t divisor) : counter(divisor), fib1(1), fib2(1){};

    std::pair<uint64_t, bool> advance(uint64_t pulses) noexcept override {
        const uint64_t distance_to_event = fib2 - value;
        const uint64_t used_pulses = std::min(distance_to_event, pulses);
        value += used_pulses;

        if (value == fib2 && fib2 != maxTreshold) {
            next_fib();
            return {used_pulses, true};
        }
        return {used_pulses, false};
    }

private:
    void next_fib() noexcept {
        if (maxTreshold - fib1 < fib2) fib2 = maxTreshold;
        else fib1 = std::exchange(fib2, fib1 + fib2);
    }
};
