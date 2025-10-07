export module counters;

import <iostream>;
import <memory>;
import <regex>;

import counter_manager;
import counter;

namespace {
using std::stoull, std::regex_match, std::regex, std::string, std::cin, std::cout, std::cerr, std::smatch, std::make_unique;

bool deleteMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex del(R"(^D (\d+)$)");
    uint64_t index;

    if (!regex_match(x, pattern, del))
        return true;

    try {
        index = stoull(pattern[1]);
    } catch (...) {
        return true;
    }

    if (manager.contains(index)) { 
        manager.erase(index);
        return false;
    }
    return true;
}

bool printMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex print(R"(^P (\d+)$)");
    uint64_t index;

    if (!regex_match(x, pattern, print))
        return true;

    try {
        index = stoull(pattern[1]);
    } catch (...) {
        return true;
    }

    if (manager.contains(index)) { 
        cout << "C " << index << " " << manager.get_counter_value(index) << "\n";
        return false;
    }
    return true;
}

bool addMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex add(R"(^A (\d+)$)");
    uint64_t index;

    if (!regex_match(x, pattern, add))
        return true;

    try {
        index = stoull(pattern[1]);
    } catch (...) {
        return true;
    }

    manager.advance(index);
    return false;
}

bool moduloMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex mod(R"(^M (\d+) (\d+) (\d+)$)");
    uint64_t index, divisor, modulus;

    if (!regex_match(x, pattern, mod))
        return true;

    try {
        index = stoull(pattern[1]);
        divisor = stoull(pattern[2]);
        modulus = stoull(pattern[3]);
    } catch (...) {
        return true;
    }

    return !manager.try_add_index<modulo_counter>(index, divisor, modulus);
}

bool fibMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex fib(R"(^F (\d+) (\d+)$)");
    uint64_t index, divisor;

    if (!regex_match(x, pattern, fib))
        return true;

    try {
        index = stoull(pattern[1]);
        divisor = stoull(pattern[2]);
    } catch (...) {
        return true;
    }

    return !manager.try_add_index<fibonacci_counter>(index, divisor);
}

bool geomMatch(string& x, smatch& pattern, counter_manager& manager) {
    static regex geom(R"(^G (\d+) (\d+)$)");
    uint64_t index, divisor;

    if (!regex_match(x, pattern, geom))
        return true;

    try {
        index = stoull(pattern[1]);
        divisor = stoull(pattern[2]);
    } catch (...) {
        return true;
    }

    return !manager.try_add_index<geometric_counter>(index, divisor);
}

} // namespace

int main() {
    counter_manager manager{};

    int lineNumber = 0;
    string x;
    while (getline(cin, x)) {
        bool printError = true;
        char operation = x[0];
        smatch pattern;
        lineNumber++;

        switch (operation) {
            case 'D':
                printError = deleteMatch(x, pattern, manager);
                break;
            case 'P' :
                printError = printMatch(x, pattern, manager);
                break;
            case 'A':
                printError = addMatch(x, pattern, manager);
                break;
            case 'M' :
                printError = moduloMatch(x, pattern, manager);
                break;
            case 'F' :
                printError = fibMatch(x, pattern, manager);
                break;
            case 'G' :
                printError = geomMatch(x, pattern, manager);
                break;
            default:
                printError = true;
        } 
        
        if(printError)
            cerr << "ERROR " << lineNumber << "\n";
    }
    return 0;
}
