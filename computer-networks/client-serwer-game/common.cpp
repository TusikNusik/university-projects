#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <regex>
#include <sstream>
#include <arpa/inet.h>

#include "err.h"
#include "common.h"

namespace detail {
    uint16_t readPort(char const *string) {
        char *endptr;
        errno = 0;
        unsigned long port = strtoul(string, &endptr, 10);

        if (errno != 0 || *endptr != 0 || port > UINT16_MAX) {
            syserr("not a valid port number");
        }

        return (uint16_t) port;
    }

    unsigned readNumber(char const *string) {
        char *endptr;
        errno = 0;
        unsigned long number = strtoul(string, &endptr, 10);

        if (errno != 0 || *endptr != 0) {
            syserr("not a valid number");
        }

        return (unsigned) number;
    }

    std::vector<double> getDoubles(const std::string& input) {
        std::istringstream iss(input);
        std::vector<double> values;
        double value;
        
        while (iss >> value) {
            values.push_back(value);
        }

        return values;
    }

    std::vector<double> readCoeffs(std::string message, unsigned N) {
        std::regex coeff_regex(R"(COEFF ((-?(?:\d+\.\d+|\d+)\s*)+)(?:\r\n)?$)");
        std::smatch match;
        std::vector<double> temp(N + 1, 0);
        std::string pom = message;

        if (std::regex_match(message, match, coeff_regex)) {
            while (pom[0] != ' ') {
                pom.erase(0, 1);
            }
            return getDoubles(pom);
        }

        return temp;
    }

    std::pair<bool, size_t> checkForEndline(std::vector<char>& x) {
        for (size_t i = 1; i < x.size(); ++i) {
            if (x[i - 1] == '\r' && x[i] == '\n') {
                return {true, i + 1};
            }
        }

        return {false, 0};
    }

    unsigned countSmallLetters(const std::string& playerID) {
        unsigned smallLetters = 0;
        for (size_t i = 0; i < playerID.size(); i++) {
            unsigned asciCode = (unsigned) playerID[i];
            if (asciCode > 96 && asciCode < 123) {
                smallLetters++;
            }
        }

        return smallLetters;
    }

    std::string formatDouble(double value) {
        if (std::trunc(value) == value) {
            return std::to_string(static_cast<long long>(value));
        } 
        else {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(7) << value;
            std::string result = oss.str();

            result.erase(result.find_last_not_of('0') + 1);

            if (result.back() == '.') {
                result.pop_back();
            }
            
            return result;
        }
    }

    std::string formatClientAddress(const sockaddr_storage& addr) {
        char ipStr[INET6_ADDRSTRLEN];
        uint16_t port;

        if (addr.ss_family == AF_INET) {
            const sockaddr_in* s = (const sockaddr_in*)&addr;
            inet_ntop(AF_INET, &(s->sin_addr), ipStr, sizeof(ipStr));
            port = ntohs(s->sin_port);
        } 
        else if (addr.ss_family == AF_INET6) {
            const sockaddr_in6* s = (const sockaddr_in6*)&addr;
            inet_ntop(AF_INET6, &(s->sin6_addr), ipStr, sizeof(ipStr));
            port = ntohs(s->sin6_port);
        } 
        else {
            return "UNKNOWN_ADDRESS";
        }

        std::ostringstream oss;
        oss << "[" << ipStr << "]" << ":" << port;
        return oss.str();
    }

}


template <typename T> 
void dataAppearedValidation(std::vector<bool>& x, size_t index, T& data, T dataToCopy) {
    if (x[index]) {
        if (data != dataToCopy) {
            syserr("Wrong Input Data!");
        }
    }
    else {
        data = dataToCopy;
    }

    x[index] = true;
}

template void dataAppearedValidation<unsigned short>(std::vector<bool>&, size_t, unsigned short&, unsigned short);
template void dataAppearedValidation<unsigned int>(std::vector<bool>&, size_t, unsigned int&, unsigned int);
template void dataAppearedValidation<std::string>(std::vector<bool>&, size_t, std::string&, std::string);
