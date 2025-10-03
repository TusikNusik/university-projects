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


#include "logs.h"


namespace logInfo {

    void printTime(uint64_t time) {
        std::cout << "[" << time/1000 << "s]" << " ";
    }

    void printConnected(sockaddr_storage& addr, socklen_t addr_len, uint64_t time) {
        std::vector<char> host(NI_MAXHOST);
        std::vector<char> service(NI_MAXSERV);

        getnameinfo(
            (sockaddr*)&addr, addr_len,
            host.data(), host.size(),
            service.data(), service.size(),
            NI_NUMERICHOST | NI_NUMERICSERV
        );

        printTime(time);
        std::cout << "Connected to: [" << host.data() << "]:" << service.data() << std::endl;
    }

    void printPutValue(unsigned point, double value, uint64_t time) {
        printTime(time);
        std::cout << "Putting " << value << " in " << point << std::endl;
    }
    
    void printStateReceived(const std::vector <double>& states, uint64_t time) {
        printTime(time);
        std::cout << "Received state";
        for (size_t i = 0; i < states.size(); i++) {
            std::cout << " " << states[i];
        }

        std::cout << std::endl;
    }

    void printCoeffsReceived(const std::vector <double>& states, uint64_t time) {
        printTime(time);
        std::cout << "Received coefficients";
        for (size_t i = 0; i < states.size(); i++) {
            std::cout << " " << states[i];
        }

        std::cout << std::endl;
    }

    void printBadPutReceived(unsigned point, double value, uint64_t time) {
        printTime(time);
        std::cout << "Received bad put with " << value << " at " << point << std::endl;
    }

    void printPenaltyReceived(unsigned point, double value, uint64_t time) {
        printTime(time);
        std::cout << "Penalty, didn't wait long enought with PUT: " << value << " at " << point << std::endl;
    }

    void printScoringReceived(const std::string& mess, uint64_t time) {
        printTime(time);
        std::cout << mess;
    }
    // --------------------------------------------SERWER COMMS--------------------------------------------------------

    void printOldNewClient(const std::string& IP, uint16_t port, uint64_t time) {
        printTime(time);
        std::cout << "New client [" << IP << "]: " << port << std::endl;
    }

    void printNewClient(const std::string& IP, uint64_t time) {
        printTime(time);
        std::cout << "New client " << IP << std::endl;
    }

    void printKnownClient(int fd, const std::string& name, uint64_t time) {
        printTime(time);
        std::cout << "File Descriptor [" << fd << "] is known as " << name << std::endl;
    }

    void printCoeffsSend(const std::vector <double>& coeffs, const std::string& name, uint64_t time) {
        printTime(time);
        std::cout << "Player " << name << " gets coefficients ";
        for (size_t i = 0; i < coeffs.size(); i++) {
            std::cout << " " << coeffs[i];
        }

        std::cout << std::endl;
    }

    void printPutSend(const std::string& name, unsigned point, double value, uint64_t time) {
        printTime(time);
        std::cout << "Player " << name << " puts " << value << " in " << point << std::endl;
    }

    void printStateSend(const std::string& mess, const std::string& name, uint64_t time) {
        printTime(time);
        std::cout << "Sending " << mess << " to player " << name;
        std::cout << std::endl;
    }

    void printScoringSend(const std::string& mess, uint64_t time) {
        printTime(time);
        std::cout << "Game end, " << mess;
    }
}