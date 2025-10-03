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
#include <fcntl.h>

#include "err.h"
#include "common.h"
#include "logs.h"

using namespace std;

#define CONNECTIONS 1024
#define BUFFER_SIZE 4096
#define NO_DELAY 0
#define HELLO_TIME 3000
#define ONE_SECOND 1000

enum class MessageType {STATE, SCORING, BAD_PUT, PENALTY, HELLO, NONE};

struct writeData {
    vector<char> buffer;
    size_t offset, length;
};

struct readData {
    vector<char> buffer;
    size_t offset;
};


class ServerData {
    private:
        uint16_t port;
        unsigned K, N, M;
        string fileName;
        ifstream file;
        sockaddr_in socketIPv4;
        sockaddr_in6 socketIPv6;
        unsigned putCount, activeClients;
        // 'polyData[i].first' to otrzymany przez i-tego gracza wielomian, a 'polyData[i].second' to jego aproksymacje.
        // 'userTime' odpowiada za sprawdzenie czy HELLO doszło w czasie 3s od połączenia.
        // 'userMess' odpowiada za wysyłania komunikatów do klienta z odpowiednim opóźnieniem.
        vector<pair<vector<double>, vector<double>>> polyData;
        vector<pair<bool, string>> userID;
        vector<pair<bool, uint64_t>> userTime;
        vector<deque<pair<pair<string, uint64_t>, MessageType>>> userMess;
        vector<writeData> serverWrite;
        vector<readData> serverRead;
        vector<unsigned> penalties;
        vector<bool> putAvailable;
        vector<sockaddr_storage> clientAddresses;

    private:
        double calculateResult(const pair<vector<double>, vector<double>>& x) {
            const vector<double>& polyCoeffs = x.first;
            const vector<double>& userPredictions = x.second;
            double result = 0;
            for (size_t i = 0; i <= K; i++) {
                double polyResult = polyCoeffs[0];
                double userResult = userPredictions[i];
                unsigned x = i;

                for (size_t j = 1; j <= N; j++) {
                    polyResult += x * polyCoeffs[j];
                    x = x * x;
                }

                double tempResult = (polyResult - userResult)*(polyResult - userResult);
                result += tempResult;
            }

            return result;
        }

    public:
        ServerData(uint16_t p, unsigned k, unsigned n, unsigned m, string f, sockaddr_in s4, sockaddr_in6 s6)
        : port(p), K(k), N(n), M(m), fileName(f), socketIPv4(s4), socketIPv6(s6), putCount(0), activeClients(0) {
            resizeBuffors(CONNECTIONS / 2);
            file.open(fileName, ifstream::in);
            if (!file.is_open()) {
                syserr("Plik nie został otworzony");
            }
        }


        sockaddr_in& getSocketIPv4() { return socketIPv4; }
        sockaddr_in6& getSocketIPv6() { return socketIPv6; }
        const vector<double>& getCoeffs(size_t index) { return polyData[index].first; }
        writeData& getWriteData(size_t index) { return serverWrite[index]; }
        readData& getReadData(size_t index) { return serverRead[index]; }
        const string& getPlayerID(size_t index) { return userID[index].second; }
        deque<pair<pair<string, uint64_t>, MessageType>>& getDelayed(size_t index) { return userMess[index]; }
        bool getPutAvailable(size_t index) { return putAvailable[index]; }
        unsigned getActiveClients() { return activeClients; }
        const sockaddr_storage& getClientAddress(size_t index) { return clientAddresses[index]; }

        void increaseActiveClients() { activeClients++; }
        void decreaseActiveClients() { activeClients--; }
        void setPutAvailable(size_t index) { putAvailable[index] = true; }
        void disablePutAvailable(size_t index) { putAvailable[index] = false; }
        bool emptyQueue(size_t index) { return userMess[index].empty() ? true : false; }
        void setClientAddress(size_t index, const sockaddr_storage& addr) { clientAddresses[index] = addr; }       


        vector<double>& getApprox(size_t index) {
            return polyData[index].second;
        }

        void delayWrite(const string& x, uint64_t time, size_t index, MessageType messType) {
            if (userMess[index].front().first.second <= time) {
                userMess[index].push_back({{x, time}, messType});
            }
            else {
                userMess[index].push_front({{x, time}, messType});
            }
        }

        // Funkcja odnotowuje zaistnienie HELLO, czyta z pliku COEFFS i zwraca wielomian przeczytany.
        string processHello(string& x, size_t index, uint64_t time) {
            userID[index].second = x;
            userTime[index].first = true;
            string coeffs;
            getline(file, coeffs);
            polyData[index].first = detail::readCoeffs(coeffs, N);
            logInfo::printCoeffsSend(polyData[index].first, userID[index].second, time);
            putAvailable[index] = true;
            return coeffs;
        }

        void registerClient(size_t index, uint64_t time) {
            userID[index].first = true;
            userTime[index] = {false, time};
        }

        bool helloHappened(size_t index, uint64_t time) {
            if (!userTime[index].first && userID[index].first) {
                if (time > userTime[index].second + HELLO_TIME) {
                    return false;
                }
            }
            return true;
        }

        bool processPut(size_t index, unsigned x, double value) {
            if (value < -5 || value > 5 || x > K) {
                givePenalty(index, 10);
                return false;
            }

            putCount++; 
            polyData[index].second[x] += value;
            return true;
        }

        bool returnResults() {
            if (M == putCount) {
                return true;
            }

            return false;
        }

        vector<pair<string, double>> sendResults() {
            vector<pair<string, double>> results;

            for (size_t i = 0; i < polyData.size(); i++) {
                if (userID[i].first) {
                    results.push_back({userID[i].second, calculateResult(polyData[i]) + penalties[i]});
                }
            }

            sort(results.begin(), results.end());

            return results;
        }

        void givePenalty(size_t index, unsigned penalty) {
            penalties[index] += penalty;
        }

        void deleteUser(size_t index) {
            polyData[index].first.clear();
            polyData[index].second.clear();
            userTime[index] = {false, 0};
            userID[index].first = false;
            userID[index].second = "";
            userMess[index].clear();

            polyData[index].first.resize(N + 1, 0);
            polyData[index].second.resize(K + 1, 0);
            writeData tempW = {vector<char>(BUFFER_SIZE), 0, 0};
            readData tempR = {vector<char>(BUFFER_SIZE), 0};
            serverWrite[index] = tempW;
            serverRead[index] = tempR;
            putAvailable[index] = false;
            penalties[index] = 0;
        }

        void restartGame() {
            size_t currentSize = polyData.size();
            polyData.clear();
            userID.clear();
            userTime.clear();
            userMess.clear();
            serverWrite.clear();
            serverRead.clear();
            penalties.clear();
            putAvailable.clear();
            clientAddresses.clear();
            resizeBuffors(currentSize / 2);
            putCount = 0;
        }

        void resizeBuffors(size_t currentSize) {
            polyData.resize(currentSize * 2, {vector<double>(N + 1, 0), vector<double>(K + 1, 0)});
            userID.resize(currentSize * 2);
            userTime.resize(currentSize * 2);
            userMess.resize(currentSize * 2, {});
            serverWrite.resize(currentSize * 2, {vector<char>(BUFFER_SIZE), 0, 0});
            serverRead.resize(currentSize * 2, {vector<char>(BUFFER_SIZE), 0});
            penalties.resize(currentSize * 2, 0);
            putAvailable.resize(currentSize * 2, false);
            clientAddresses.resize(currentSize * 2);
        }
};

namespace helperServer {
    void setDeafulServertValues(uint16_t& p, unsigned& k, unsigned& n, unsigned& m ) {
        p = 0;
        k = 100;
        n = 4;
        m = 131;
    } 
    
    uint64_t timeSinceStart() {
        static auto startTime = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();
    }

    template <typename T>
    string vectorToString(string mess, const vector<T>& x) {
        ostringstream oss;
        oss << mess;
        for (const auto& el : x) {
            oss << " " << detail::formatDouble(el);
        }

        oss << "\r\n";
        return oss.str();
    }

    template <typename T1, typename T2>
    string vectorToString(const string& mess, const vector<pair<T1, T2>>& x) {
        ostringstream oss;
        oss << mess;
        for (const auto& el : x) {
            oss << " " << el.first << " " << detail::formatDouble(el.second);
        }

        oss << "\r\n";
        return oss.str();
    }

    void closeConnection(ServerData& server, vector<pollfd>& fds, size_t index) {
        close(fds[index].fd);
        fds[index].fd = -1;
        server.deleteUser(index);
        server.decreaseActiveClients();
    }
}

pair<int, int> socketConfig(ServerData& server) {
    int socket_fdV4 = socket(AF_INET, SOCK_STREAM, 0);
    int socket_fdV6 = socket(AF_INET6, SOCK_STREAM, 0);

    if (socket_fdV4 < 0 || socket_fdV6 < 0) {
        syserr("cannot create a socket");
    }

    if (fcntl(socket_fdV4, F_SETFL, O_NONBLOCK) || fcntl(socket_fdV6, F_SETFL, O_NONBLOCK)) {
        syserr("fcntl");
    }

    int opt = 1;
    if (setsockopt(socket_fdV6, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
        syserr("setsockopt IPV6_V6ONLY failed");
    }

    if (bind(socket_fdV4, (struct sockaddr *) &server.getSocketIPv4(), (socklen_t) sizeof server.getSocketIPv4()) < 0) {
        syserr("SocketIPv4 binding failed");
    }

    
    if (bind(socket_fdV6, (struct sockaddr *) &server.getSocketIPv6(), (socklen_t) sizeof server.getSocketIPv6()) < 0) {
        syserr("SocketIPv6 binding failed");
    }

    if (listen(socket_fdV4, SOMAXCONN) < 0) {
        syserr("listen IPV4");
    }

    if (listen(socket_fdV6, SOMAXCONN) < 0) {
        syserr("listen IPV6");
    }

    socklen_t lenght = (socklen_t) sizeof server.getSocketIPv4();
    if (getsockname(socket_fdV4, (struct sockaddr *) &server.getSocketIPv4(), &lenght) < 0) {
        syserr("getsockname");
    }

    return {socket_fdV4, socket_fdV6};
}

// W tej funkcji index ma tylko wartość 0 lub 1, odpowiednio klient łączony do gniazda IPv4, IPv6.
bool acceptNewClient(ServerData& server, vector<pollfd>& fds, size_t index) {
    sockaddr_storage clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    if (fds[index].revents & POLLIN) {
        int client_fd = accept(fds[index].fd, (struct sockaddr *) &clientAddress, &clientLen);

        if (client_fd < 0) {
           if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            } 
            else {
                error("accept failed");
                return false;
            }
        }

        if (fcntl(client_fd, F_SETFL, O_NONBLOCK)) {
            syserr("fcntl");
        }

        bool accepted = false;
        size_t clientIndex = 2;

        for (size_t i = 2; i < fds.size(); i++) {
            if (fds[i].fd == -1) {
                fds[i].fd = client_fd;
                fds[i].events = POLLIN | POLLOUT;
                accepted = true;
                clientIndex = i;
                break;
            }
        }

        // Maksymalnia ilość klientów została osiągnieta, konieczny resize.
        if (!accepted) {
            size_t fdsSize = fds.size();
            clientIndex = fdsSize;
            fds.resize(fds.size() * 2);
            fds[fdsSize].fd = client_fd;
            fds[fdsSize].events = POLLIN | POLLOUT;
            server.resizeBuffors(fdsSize);
        }
        
        server.increaseActiveClients();
        server.setClientAddress(clientIndex, clientAddress);
        logInfo::printNewClient(detail::formatClientAddress(clientAddress), helperServer::timeSinceStart());

        server.registerClient(clientIndex, helperServer::timeSinceStart());
        return true;
    }
    return false;
}

void delayMessage(ServerData& server, string& x, size_t index, uint64_t delay, MessageType messType) {
    server.delayWrite(x, helperServer::timeSinceStart() + delay, index, messType);
}

// Przygotowanie do zakończenia gry, zapełnia kolejke każdego klienta wiadomością SCORING.
void setScores(ServerData& server, vector<pollfd>& fds) {
    vector<pair<string, double>> results = server.sendResults();
    string mess = helperServer::vectorToString("SCORING", results);
    logInfo::printScoringSend(mess, helperServer::timeSinceStart());

    for (size_t i = 2; i < fds.size(); i++) {
        if (fds[i].fd != -1) {
            delayMessage(server, mess, i, NO_DELAY, MessageType::SCORING);
        }
    }
}

// Przygotowanie danych, przekazywanie danych do wysłania, które są gotowe, fragmentacja zgodna z wielkością bufora.
void prepareBuffer(ServerData& server, size_t index, bool endGame) {
    writeData& temp = server.getWriteData(index);
    deque<pair<pair<string, uint64_t>, MessageType>>& delayedMess = server.getDelayed(index);
    MessageType messType = MessageType::NONE;

    // Pętla wykonuje się dopóki kolejka nie jest pusta ORAZ pierwsza wiadomość jest gotowa do wysłania ORAZ bufor wysyłajacy nie jest zapełniony.
    while(!delayedMess.empty() && delayedMess.front().first.second < helperServer::timeSinceStart() && temp.length != BUFFER_SIZE) {
        string mess = delayedMess.front().first.first;
        string copyMess = mess;
        messType = delayedMess.front().second;
        bool pushRest = false;

        // Wiadomość w pełni zostaję przekazana.
        if (mess.size() < BUFFER_SIZE - temp.length) {
            copy(mess.begin(), mess.end(), temp.buffer.begin() + temp.length);
            temp.length += mess.size();
        }       
        else {
            copy(mess.begin(), mess.begin() + BUFFER_SIZE - temp.length, temp.buffer.begin() + temp.length);
            mess.erase(0, BUFFER_SIZE - temp.length);
            temp.length = BUFFER_SIZE;
            pushRest = true;
        }
        
        delayedMess.pop_front();
        if (pushRest) {
            delayedMess.push_front({{mess, NO_DELAY}, MessageType::NONE});
        }


        if (copyMess.size() > 1) {
            copyMess.erase(copyMess.size() - 2, copyMess.size() - 1);
        }

        if (messType == MessageType::STATE) {
            server.setPutAvailable(index);
            if(!endGame) {
                logInfo::printStateSend(copyMess, server.getPlayerID(index), helperServer::timeSinceStart());
            }
        }

        if (messType == MessageType::PENALTY) {
            server.setPutAvailable(index);
        }

        if (messType == MessageType::BAD_PUT) {
            server.setPutAvailable(index);
        }

    }
}


void readMessage(ServerData& server, size_t index) {
    static const regex helloRegex(R"(^HELLO ([a-zA-Z0-9]+)\r\n$)");
    static const regex putRegex(R"(^PUT (\d+) (-?\d+(?:\.\d{0,7})?)\r\n$)");

    readData& tempRead = server.getReadData(index);

    pair<bool, size_t> contRead = detail::checkForEndline(tempRead.buffer);

    while(contRead.first) {
        string pattern(tempRead.buffer.begin(), tempRead.buffer.begin() + contRead.second);
        smatch match;

        if (regex_match(pattern, match, helloRegex)) {
            string playerID = match[1].str();
            string mess = server.processHello(playerID, index, helperServer::timeSinceStart()) + "\r\n";

            delayMessage(server, mess, index, NO_DELAY, MessageType::HELLO);
        }
        else if (regex_match(pattern, match, putRegex)) {
            unsigned x = stoi(match[1].str());
            double value = stod(match[2].str());
            logInfo::printPutSend(server.getPlayerID(index), x, value, helperServer::timeSinceStart());

            if (server.getPutAvailable(index)) {
                bool checkParams = server.processPut(index, x, value);
                if (!checkParams) {
                    vector<double> temp = {(double)x, value};
                    string mess = helperServer::vectorToString("BAD_PUT", temp);
                    delayMessage(server, mess, index, ONE_SECOND, MessageType::BAD_PUT);
                }
                else {
                    const string& playerID = server.getPlayerID(index);
                    unsigned delay = detail::countSmallLetters(playerID) * ONE_SECOND;
                    vector<double>& coefs = server.getApprox(index);
                    string mess = helperServer::vectorToString("STATE", coefs);
                    delayMessage(server, mess, index, delay, MessageType::STATE);
                }
            }
            else {
                server.givePenalty(index, 20);
                vector<double> temp = {(double)x, value};
                string mess = helperServer::vectorToString("PENALTY", temp);
                delayMessage(server, mess, index, NO_DELAY, MessageType::PENALTY);
            }
            server.disablePutAvailable(index);
        }   
        else {
            string player = server.getPlayerID(index);
            if(player == "") {
                player = "UNKNOWN";
            }

            string badMess = "bad message from " + detail::formatClientAddress(server.getClientAddress(index));
            badMess += ", " + player + ": " + pattern;
            error(badMess);
        }
        
        move(tempRead.buffer.begin() + contRead.second, tempRead.buffer.begin() + BUFFER_SIZE - contRead.second, tempRead.buffer.begin());
        tempRead.offset -= contRead.second;
        contRead = detail::checkForEndline(tempRead.buffer);
    } 
}

ServerData entryInfo(int argc, char *argv[]) {
    uint16_t port;
    unsigned K, N, M;
    sockaddr_in socketIPv4;
    sockaddr_in6 socketIPv6;
    string file;

    helperServer::setDeafulServertValues(port, K, N, M);

    vector<bool> paramsAppeared(5, false);  // 0: p_appeared, 1: k_appeared, 2: n_appeared, 3: m_appeared, 4: f_appeared;                 

    string compare;
    for (int i = 1; i < argc; i += 2) {   
        compare = string(argv[i]);

        if (compare == "-p" && i + 1 < argc) {
            uint16_t portTemp = detail::readPort(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 0, port, portTemp);
        }
        else if (compare == "-k" && i + 1 < argc) {
            unsigned kTemp = detail::readNumber(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 1, K, kTemp);
        }
        else if (compare == "-n" && i + 1 < argc) {
            unsigned nTemp = detail::readNumber(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 2, N, nTemp);
        }
        else if (compare == "-m" && i + 1 < argc) {
            unsigned mTemp = detail::readNumber(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 3, M, mTemp);
        }
        else if (compare == "-f" && i + 1 < argc) {
            string fileTemp = string(argv[i + 1]);
            dataAppearedValidation(paramsAppeared, 4, file, fileTemp);
        }
        else {
            syserr("Unrecognized param.");
        }
    }

    memset(&socketIPv4, 0, sizeof(socketIPv4));
    memset(&socketIPv6, 0, sizeof(socketIPv6));

    socketIPv4.sin_port = htons(port);
    socketIPv4.sin_addr.s_addr = INADDR_ANY;
    socketIPv4.sin_family = AF_INET;
    socketIPv6.sin6_port = htons(port);
    socketIPv6.sin6_family = AF_INET6;
    socketIPv6.sin6_addr = in6addr_any;

    ServerData tempServer(port, K, N, M, file, socketIPv4, socketIPv6);

    return tempServer;
}


int main(int argc, char *argv[]) {
    helperServer::timeSinceStart();
    ServerData server = entryInfo(argc, argv);

    pair<int, int> fdPair = socketConfig(server);
    bool endGame = false;

    int socket_fdV4 = fdPair.first;
    int socket_fdV6 = fdPair.second;

    vector<pollfd> fds(CONNECTIONS);

    // Główny socket IPv4 w komórce '0', odpowiednik IPv6 w '1'.
    fds[0].fd = socket_fdV4;
    fds[0].events = POLLIN;
    fds[1].fd = socket_fdV6;
    fds[1].events = POLLIN;

    for (size_t i = 2; i < fds.size(); i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN | POLLOUT;
    }

    while(true) {
        if (endGame && server.getActiveClients() == 0) {
            endGame = false;
            server.restartGame();
            this_thread::sleep_for(chrono::seconds(1));
        }
        
        if (!endGame) {
            for (size_t i = 2; i < fds.size(); i++) {
                fds[i].revents = 0;
            }
            
            int poll_status = poll(fds.data(), fds.size(), 100); // 100 ms timeout

            if (poll_status < 0) {
                error("poll failed");
                continue;
            }

            if (poll_status == 0) {
                continue;
            }
            
            while(acceptNewClient(server, fds, 0) || acceptNewClient(server, fds, 1)) {
            }
        }

        for (size_t i = 2; i < fds.size(); i++) {
            if(fds[i].fd != -1 && !server.helloHappened(i, helperServer::timeSinceStart())) {
                helperServer::closeConnection(server, fds, i);
                continue;
            }

            if (!endGame && fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                readData& temp = server.getReadData(i);
                ssize_t recBytes = read(fds[i].fd, temp.buffer.data() + temp.offset, BUFFER_SIZE);

                if (recBytes <= 0) {
                    error("error when reading message from connection");
                    helperServer::closeConnection(server, fds, i);
                } 
                else {
                    temp.offset += recBytes;
                    readMessage(server, i);
                }

                endGame = server.returnResults();
                if (endGame) {
                    setScores(server, fds);
                }
            }

            if (fds[i].fd != -1 && (fds[i].revents & POLLOUT)) {
                writeData& temp = server.getWriteData(i);
                prepareBuffer(server, i, endGame);

                if (temp.length > 0) {
                    ssize_t sentBytes = write(fds[i].fd, temp.buffer.data(), temp.length);

                    if (sentBytes < 0) {
                        error("error when writing message");
                        helperServer::closeConnection(server, fds, i);
                    }
                    else {
                        move(temp.buffer.begin() + sentBytes, temp.buffer.begin() + temp.length, temp.buffer.begin());
                        temp.length -= sentBytes;
                    }
                    if (endGame && temp.length == 0 && server.emptyQueue(i)) {
                        helperServer::closeConnection(server, fds, i);
                    }
                }
            }
        }
    }
}