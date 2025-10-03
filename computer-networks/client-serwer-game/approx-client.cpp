#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <bits/stdc++.h>
#include <chrono>
#include <poll.h>
#include <regex>


#include "err.h"
#include "common.h"
#include "logs.h"

#define BUFFER_SIZE 4096

using namespace std;

struct clientProtocol {
    sockaddr_storage addr;
    socklen_t addr_len;
    bool isIPv4;
};

struct readData {
    vector<char> buffer;
    size_t offset;
};

class ClientData {
    private:
        string playerID, serverIP;
        uint16_t serverPort;
        bool addressIPv4, clientStrategy;
        sockaddr_storage socket;
        socklen_t socketLen;
        bool receivedCoeffs, receivedAnswer, gentleShutdown;
        deque<string> delayedMess;
        vector<double> coeffs;
        vector<double> predictions;
        unsigned maxK;

    public:
        ClientData(string pID, string sIP, uint16_t p, bool s, clientProtocol cd) :
        playerID(pID), serverIP(sIP), serverPort(p), clientStrategy(s), receivedCoeffs(false),
        receivedAnswer(true), gentleShutdown(false) {
            socket = cd.addr;
            socketLen = cd.addr_len;
            addressIPv4 = cd.isIPv4;
            maxK = 10000;
            predictions.clear();
            predictions.resize(10002, 0);
        }


        bool isIPv4() { return addressIPv4; }
        bool isClientStrategy() { return clientStrategy; }
        const string& getServerData() { return serverIP; }
        const string& getPlayerID() { return playerID; }
        uint16_t getPort() { return serverPort; }
        sockaddr_storage& getSocket() { return socket; }
        socklen_t getSocketSize() { return socketLen; }
        bool getReceivedCoeffs() { return receivedCoeffs; }
        void setReceivedAnswer() { receivedAnswer = true; }
        bool getReceivedAnswer() { return receivedAnswer; }
        vector<double> getPrediction() { return predictions; }
        bool getGentleShutdown() { return gentleShutdown; }
        unsigned getMaxK() { return maxK; }

        void gentleShutdownAppeared() { gentleShutdown = true; }
        void sendPut() { receivedAnswer = false; }
        void setReceivedCoeffs() { receivedCoeffs = true; }
        void addDelayedMess(string& x) { delayedMess.push_front(x); }
        void changePredictions(size_t index, double value) { predictions[index] += value; }
        void changeMaxK(unsigned x) { maxK = x; }

        void applyCoeffs(vector<double> x) {
            coeffs = x;
        }
        
        double calculatePoly(unsigned until) {
            if(coeffs.size() == 0)
                return 0;

            double polyResult = coeffs[0];
            unsigned x = until;

            for (size_t j = 1; j < coeffs.size(); j++) {
                polyResult += x * coeffs[j];
                x = x * x;
            }

            return polyResult;
        }
};

namespace helper {
    uint64_t timeSinceStart() {
        static auto startTime = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();
    }

    void sendMessage(int fd, string& mess) {
        size_t fullMessage = mess.size();
        size_t alreadySend = 0;
        while(alreadySend < fullMessage) {
            ssize_t sentBytes = write(fd, mess.data() + alreadySend, fullMessage);

            if (sentBytes < 0) {
                error("write error");
                return;
            }

            if (sentBytes == 0) {
                error("unexpected server disconnect");
                close(fd);
                exit(1);
            }

            alreadySend += sentBytes;
            fullMessage -= sentBytes;
        }
    }

    void copyData(readData& readData, vector<char>& flex) {
        flex.insert(flex.end(), readData.buffer.begin(), readData.buffer.begin() + readData.offset);
        readData.buffer.clear();
        readData.buffer.resize(BUFFER_SIZE);
        readData.offset = 0;
    }
}

clientProtocol chooseProtocol(const string& serwerIP, uint16_t port, vector<bool>& paramsAppeared) {
    sockaddr_storage addr;
    socklen_t addr_len = 0;
    bool isIPv4 = false;

    struct addrinfo hints{}, *res;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;

    if (paramsAppeared[3]) {
        hints.ai_family = AF_INET;
    }
    else if (paramsAppeared[4]) {
        hints.ai_family = AF_INET6;
    }
    else {
        hints.ai_family = AF_UNSPEC;
    }

    int err = getaddrinfo(serwerIP.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (err != 0) {
        syserr(("getaddrinfo: " + std::string(gai_strerror(err))).c_str());
    }

    bool found = false;
    for (struct addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
        if (!paramsAppeared[3] && !paramsAppeared[4]) {
            if (rp->ai_family == AF_INET6) {
                isIPv4 = false;
            } else if (rp->ai_family == AF_INET) {
                isIPv4 = true;
            } else {
                continue;
            }
        } else {
            isIPv4 = (rp->ai_family == AF_INET);
        }

        std::memcpy(&addr, rp->ai_addr, rp->ai_addrlen);
        addr_len = rp->ai_addrlen;
        found = true;
        break;
    }

    freeaddrinfo(res);

    if (!found) {
        syserr("No suitable address found.");
    }

    clientProtocol temp;
    temp.addr = addr;
    temp.addr_len = addr_len;
    temp.isIPv4 = isIPv4;
    return temp;
}

ClientData entryInfo(int argc, char *argv[]) {
    uint16_t port;
    string playerID;
    string serwerIP;

    vector<bool> paramsAppeared(5, false);  // 0: u_appeared, 1: s_appeared, 2: p_appeared, 3: 4_appeared, 4: 6_appeared, 5: a_appeared;                 

    string compare;
    bool nextValid = false;
    for (int i = 1; i < argc; i++) {   
        compare = string(argv[i]);

        if (compare == "-p" && i + 1 < argc) {
            uint16_t portTemp = detail::readPort(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 2, port, portTemp);
            nextValid = true;
        }
        else if (compare == "-u" && i + 1 < argc) {
            string userTemp = string(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 0, playerID, userTemp);
            nextValid = true;
        }
        else if (compare == "-s" && i + 1 < argc) {
            string serverTemp = string(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 1, serwerIP, serverTemp);
            nextValid = true;
        }
        else if (compare == "-4") {
            paramsAppeared[3] = true;
        }
        else if (compare == "-6") {
            paramsAppeared[4] = true;
        }
        else if (compare == "-a") {
            paramsAppeared[5] = true;
        }
        else {
            if (nextValid) {
                nextValid = false;
            }
            else {
                syserr("Unrecognized param.");
            }
        }
    }
    if (!(paramsAppeared[0] && paramsAppeared[1] && paramsAppeared[2])) {
        syserr("Not enought params.");
    }

    clientProtocol clientProtocol = chooseProtocol(serwerIP, port, paramsAppeared);

    ClientData temp(playerID, serwerIP, port, paramsAppeared[5], clientProtocol);

    return temp;
}

void setCommunication(ClientData& client, vector<pollfd>& fds) {
    int sockfd = -1;
    sockaddr_storage& socketMain = client.getSocket();
    socklen_t socketSize = client.getSocketSize();

    if (client.isIPv4()) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    else {
        sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    }

    if (sockfd < 0) {
        syserr("cannot create a socket");
    }

    if (connect(sockfd, (struct sockaddr*)&socketMain, socketSize) < 0) {
        syserr("cannot connect to the server");
    }

    logInfo::printConnected(socketMain, socketSize, helper::timeSinceStart());

    if (!client.isClientStrategy()) {
        fds[0].fd = STDIN_FILENO;   // Wejście użytkownika.
        fds[0].events = POLLIN;
    }

    fds[1].fd = sockfd;         // Socket TCP.
    fds[1].events = POLLIN | POLLOUT;
}

void handleInput(ClientData& client, vector<pollfd>& fds) {
    static const regex putRegex(R"(^(\d+) (-?\d+(?:\.\d{1,7})?)$)");

    if (fds[0].revents & POLLIN) {
        string input;
        smatch match;

        getline(cin, input);

        if (regex_match(input, match, putRegex)) {
            string mess = "PUT " + match[0].str() + "\r\n";

            if (client.getReceivedCoeffs()) {
                helper::sendMessage(fds[1].fd, mess);
                client.sendPut();
                unsigned x = stoi(match[1].str());
                double value = stod(match[2].str());
                logInfo::printPutValue(x, value, helper::timeSinceStart());
            }
            else {
                client.addDelayedMess(mess);
            }
        }
        else {
            string messInvalid = "invalid input line " + input;
            error(messInvalid);
        }
    }
}

void handleStrategy(ClientData& client, vector<pollfd>& fds) {
    if (!client.getReceivedCoeffs() || !client.getReceivedAnswer()) {
        return;
    }

    vector<double> predictions = client.getPrediction();
    for (size_t i = 0; i <= client.getMaxK(); i++) {
        double calculatePol = client.calculatePoly(i);

        if (predictions[i] != calculatePol) {
            double difference = calculatePol - predictions[i];

            if (difference > 5) {
                difference = 5;
            }
            if (difference < -5) {
                difference = -5;
            }

            client.changePredictions(i, difference);
            string mess = "PUT " + to_string(i) + " " + to_string(difference) + "\r\n";
            helper::sendMessage(fds[1].fd, mess);
            logInfo::printPutValue(i, difference, helper::timeSinceStart());
            client.sendPut();
            return;
        }
    }
}

vector<double> polyParams(string& message) {
    static const regex numberRegex(R"(-?\d+(?:\.\d{1,7})?)");
    vector<double> values;

    auto begin = sregex_iterator(message.begin(), message.end(), numberRegex);
    auto end = sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        values.push_back(stod(it->str()));
    }

    return values;
}

ssize_t readMessage(readData& readData, vector<char>& flex, vector<pollfd>& fds) {
    if (readData.offset != 0) {
        helper::copyData(readData, flex);
    }

    ssize_t readBytes = read(fds[1].fd, readData.buffer.data(), BUFFER_SIZE);
    
    if (readBytes < 0) {
        error("read error");
        return -1;
    }

    if (readBytes == 0) {
        error("unexpected server disconnect \n");
        close(fds[0].fd);
        close(fds[1].fd);
        exit(1);
    }

    readData.offset += readBytes;
    helper::copyData(readData, flex);

    return readBytes;
}

void handleServer(ClientData& client, readData& readData, vector<pollfd>& fds) {
    static const regex coeffRegex(R"(COEFF(?:\s-?\d+(?:\.\d{1,7})?)+\r\n)");
    static const regex badPutRegex(R"(BAD_PUT\s(\d+)\s(-?\d+(?:\.\d{0,7})?)\r\n)");
    static const regex penaltyRegex(R"(PENALTY\s(\d+)\s(-?\d+(?:\.\d{0,7})?)\r\n)");
    static const regex stateRegex(R"(STATE(?:\s-?\d+(?:\.\d{1,7})?)+\r\n)");
    static const regex scoringRegex(R"(SCORING(?: [a-zA-Z0-9]+ -?\d+(?:\.\d{1,7})?)+\r\n)");

    vector<char> flex;
    pair<bool, size_t> contRead = {false, 0};

    // Blokujące gniazdo czeka aż dostanie od serwera koniec linii.
    while(true) {
        ssize_t readBytes = readMessage(readData, flex, fds);
        if (readBytes < 0) {
            return;
        }

        contRead = detail::checkForEndline(flex);
        
        if (contRead.first) {
            break;
        }
    }
    
    while(contRead.first) {

        string pattern(flex.begin(), flex.begin() + contRead.second);
        smatch match;

        if (regex_match(pattern, match, coeffRegex)) {
            vector<double> poly = polyParams(pattern);
            client.setReceivedCoeffs();
            client.applyCoeffs(poly);
            logInfo::printCoeffsReceived(poly, helper::timeSinceStart());
        }
        else if (regex_match(pattern, match, badPutRegex)) {
            unsigned x = stoi(match[1].str());
            double value = stod(match[2].str());
            client.setReceivedAnswer();
            client.changeMaxK(x - 1);
            logInfo::printBadPutReceived(x, value, helper::timeSinceStart());
        }
        else if (regex_match(pattern, match, penaltyRegex)) {
            unsigned x = stoi(match[1].str());
            double value = stod(match[2].str());
            client.setReceivedAnswer();
            logInfo::printPenaltyReceived(x, value, helper::timeSinceStart());
        }
        else if (regex_match(pattern, match, stateRegex)) {
            vector<double> poly = polyParams(pattern);
            client.setReceivedAnswer();
            logInfo::printStateReceived(poly, helper::timeSinceStart());
        }
        else if (regex_match(pattern, match, scoringRegex)) {
            logInfo::printScoringReceived(pattern, helper::timeSinceStart());
            close(fds[0].fd);
            close(fds[1].fd);
            exit(0);
        }       
        else {
            string player = "UNKNOWN";
            string badMess = "bad message from " + detail::formatClientAddress(client.getSocket());
            badMess += ", " + player + ": " + pattern;
            error(badMess);
        }

        flex.erase(flex.begin(), flex.begin() + contRead.second);
        contRead = detail::checkForEndline(flex);
    }

    // Jeżeli serwer przesłał nadmiar wiadomości, zapamiętuje, aby nie gubić informacji.
    if (flex.size() != 0) {
        copy(flex.begin(), flex.end(), readData.buffer.begin());
        readData.offset += flex.size();
    }

}

int main(int argc, char *argv[]) {
    helper::timeSinceStart();

    ClientData client = entryInfo(argc, argv);

    vector<pollfd> fds(2);

    setCommunication(client, fds);

    vector<char> writeBuffer(BUFFER_SIZE);
    readData readBuffer = {vector<char>(BUFFER_SIZE), 0};

    string hello = "HELLO ";
    string mess = hello + client.getPlayerID() + "\r\n";
    helper::sendMessage(fds[1].fd, mess);

    while(true) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        int poll_status = poll(fds.data(), fds.size(), -1);

        if(poll_status < 0) {
            error("poll failed");
            continue;
        }

        if(poll_status == 0) {
            continue;
        }

        if (!client.isClientStrategy()) {
            handleInput(client, fds);
        }

        if (fds[1].revents & POLLIN) {
            handleServer(client, readBuffer, fds);
        }

        if (client.isClientStrategy()) {
            handleStrategy(client, fds);
        }
    }

    return 0;
}