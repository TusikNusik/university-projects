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

#include "err.h"
#include "common.h"


#define BUFFER_SIZE 65538

using namespace std;

// Numery portów w klasach przechowywane są w formacie Little-Endian.
// SyncNode to klasa, która przechowuje informacje: czy jest próba synchronizacji, czy aktualnie wezeł jest zsynchronizowany,
// a jezeli tak to jakie wartości jego synchronizujący węzeł.
class SyncNode {
    private:

        uint16_t port;
        struct sockaddr_in adress;
        uint8_t sync;
        bool syncToDate, tryToSync;     // tryToSync - próba synchronizacji, syncToDate - czy zsynchronizowany.

    public:
    
    SyncNode(uint16_t p, struct sockaddr_in s, uint8_t sy) : port(p), adress(s), sync(sy), syncToDate(true) {}
    SyncNode() : syncToDate(false), tryToSync(false) {
        memset(&adress, 0, sizeof(adress));
    }

    void setNode(uint16_t p, struct sockaddr_in s, uint8_t sy) {port = p; adress = s; sync = sy; }
    void setToDate(bool x) { syncToDate = x; }
    void setTrySync(bool x) { tryToSync = x; }

    bool getToDate() { return syncToDate; }
    bool getTrySync() { return tryToSync; }
    uint8_t getSync() { return sync; }
    uint16_t getPort() { return port; }
    struct sockaddr_in& getSockaddr() { return adress; }

    bool isSyncNode(uint16_t p, const struct sockaddr_in& s) {
        if(p == port && s.sin_addr.s_addr == adress.sin_addr.s_addr && (syncToDate || tryToSync) ) {
            return true;
        }

        return false;
    }
};

class Node {
    private:

        uint16_t port;  
        struct sockaddr_in own_adress;  
        bool leader, a_or_r, ignoreSync;
        uint8_t sync;
        vector <pair<uint16_t, struct sockaddr_in>> peers;
        vector <uint64_t> T = {0, 0, 0, 0, 0};     // Przechowuje T1, T2, T3, T4, na odpowiednio T[1], T[2], T[3], T[4].
        int offset;
        SyncNode syncNode;

    public:

    Node(uint16_t p, struct sockaddr_in s, bool a_or_r)
        : port(p), own_adress(s), leader(false), a_or_r(a_or_r), ignoreSync(false), sync(255), peers(), offset(0) {}

    Node() : leader(false), a_or_r(false), ignoreSync(false), sync(255), peers(), offset(0) {
        port = 0;
        memset(&own_adress, 0, sizeof(own_adress));
        own_adress.sin_family = AF_INET;
        own_adress.sin_addr.s_addr = INADDR_ANY;
        own_adress.sin_port = 0;
    }

    void addPeer(uint16_t port, struct sockaddr_in x) { // addPeer również oczekuje portu w formacie Little-Endian.
        if(!knownPeer(port, x)) {
            peers.push_back(make_pair(port, x));
        }
    }

    bool knownPeer(uint16_t port, const struct sockaddr_in& x) {      
        for(size_t i = 0; i < peers.size(); i++) {
            if(port == peers[i].first && x.sin_addr.s_addr == peers[i].second.sin_addr.s_addr) {
                return true;
            }
        }

        return false;
    }

    struct sockaddr_in& getSockaddr() { return own_adress; }
    uint16_t getPort() { return port; }
    bool getHelloApproval() { return a_or_r; }
    const vector<pair<uint16_t, struct sockaddr_in>>& getPeers() { return peers; }
    uint8_t getSync() { return sync; }
    bool getLeader() { return leader; }
    const vector<uint64_t>& getT() { return T; }
    int getOffset() { return offset; }
    bool getIgnoreSync() { return ignoreSync; }
    SyncNode& getSyncNode() { return syncNode; }

    void changeOffset(int x) { offset = x; }
    void changeT(uint64_t T_value, size_t T_index) { T[T_index] = T_value; }
    void changeLeader(bool x) { leader = x;  }
    void changeSync(uint8_t x) { sync = x; } 
    void changeIgnoreSync(bool x) { ignoreSync = x; }
    void eraseFirstPeer() { peers.erase(peers.begin()); }
    
    void changeSyncNode() { // Wywyoluje kiedy węzeł synchronizujący nie odpowiada przez 20s-30s, kończymy z nim synchronizacje.
        sync = 255;
        syncNode.setToDate(false);
        syncNode.setTrySync(false);
        leader = false;
    }
};

// Czasy odpowiednio do wysyłania SYNC_START do znanych węzłów oraz do sprawdzania czy synchronizacja dalej zachodzi.
uint64_t time5s, time20s, time9s;

Node entryInfo(int argc, char *argv[]) {

    struct sockaddr_in own_adress, peer_adress, temp;
    uint16_t port = 0, peer_port = 0;
    bool a_or_r = false;
    vector<bool> paramsAppeared(5, false);  // 0: b_appeared, 1: p_appeared, 2: a_appeared, 3: r_appeared;
    memset(&own_adress, 0, sizeof(own_adress));
    memset(&peer_adress, 0, sizeof(peer_adress));
    own_adress.sin_family = AF_INET;
    peer_adress.sin_family = AF_INET;
    own_adress.sin_addr.s_addr = INADDR_ANY;  
    own_adress.sin_port = 0;                  

    string compare;
    for(int i = 1; i < argc; i += 2) {   
        compare = string(argv[i]);
        if(compare == "-b" && i + 1 < argc) {
            char* own_adr = argv[i + 1];
            if(paramsAppeared[0]) {
                if (inet_pton(AF_INET, own_adr, &(temp.sin_addr)) <= 0) {
                    syserr("Wrong Input Data!");
                }

                if(temp.sin_addr.s_addr != own_adress.sin_addr.s_addr) {
                    syserr("Wrong Input Data!");
                }
            }
            else {  
                 // inet_pton już ustawia na porządek sieciowy.
                if (inet_pton(AF_INET, own_adr, &(own_adress.sin_addr)) <= 0) {
                    syserr("Wrong Input Data!");
                }

                paramsAppeared[0] = true;
            }
        }
        else if(compare == "-p" && i + 1 < argc) {
            port = read_port(argv[i + 1]);
            if(paramsAppeared[1]) {
                if(htons(port) != own_adress.sin_port) {
                    syserr("Wrong Input Data!");
                }
            }
            else {
                own_adress.sin_port = htons(port);
            }

            paramsAppeared[1] = true;
        }
        else if(compare == "-a" && i + 1 < argc) {
            char* peer_adr = argv[i + 1];
            if(paramsAppeared[2]) {
                if(inet_pton(AF_INET, peer_adr, &(temp.sin_addr)) <= 0) {
                    temp = get_server_address(argv[i + 1], peer_port);
                }

                if(temp.sin_addr.s_addr != peer_adress.sin_addr.s_addr) {
                    syserr("Wrong Input Data!");
                }
            }
            else {
                // Jezeli nie przechodzi adress IP to testujemy, czy nazwa hosta przechodzi.
                if(inet_pton(AF_INET, peer_adr, &(peer_adress.sin_addr)) <= 0) {
                    peer_adress = get_server_address(argv[i + 1], peer_port);
                }
            }

            paramsAppeared[2] = true;
            a_or_r = true;
        }
        else if(compare == "-r" && i + 1 < argc) {
            peer_port = read_port(argv[i + 1]);
            if(paramsAppeared[3]) {
                if(htons(peer_port) != peer_adress.sin_port) {
                    syserr("Wrong Input Data!");
                }
            }
            else {
                peer_adress.sin_port = htons(peer_port);
            }

            paramsAppeared[3] = true;
            a_or_r = true;
        }
        else {
            syserr("Wrong Input Data!");
        }
    }

    if((paramsAppeared[2] && !paramsAppeared[3]) || (!paramsAppeared[2] && paramsAppeared[3])) {
        syserr("BOTH -a AND -r SHOULD APPEAR OR NONE");
    }

    Node tempNode(port, own_adress, a_or_r);

    if(a_or_r) {
        tempNode.addPeer(peer_port, peer_adress);
    }

    return tempNode;
}

uint64_t timeSinceStart() {
    static auto startTime = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();
}

// x[index] - najbardziej znaczący bajt dla liczby 8 bajtowej w formacie Big-Endian.
void parseTime(vector<uint8_t>& x, size_t index, uint64_t T) {
    x[index] = ((T >> 56) & 0xFF);
    x[index + 1] = ((T >> 48) & 0xFF);
    x[index + 2] = ((T >> 40) & 0xFF);
    x[index + 3] = ((T >> 32) & 0xFF);
    x[index + 4] = ((T >> 24) & 0xFF);
    x[index + 5] = ((T >> 16) & 0xFF);
    x[index + 6] = ((T >> 8) & 0xFF);
    x[index + 7] = (T & 0xFF);
}

uint64_t readTime(vector<uint8_t>& x, size_t index) {
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | x[index + i];
    }

    return result;
}

void sendMessage(vector<uint8_t>& x, size_t messageSize, const struct sockaddr_in& peer_address, int socket) {
    ssize_t sent = sendto(socket, x.data(), messageSize, 0, (struct sockaddr*)&peer_address, sizeof(peer_address));
    if(sent < 0) {
        error("sendto failed.");
    }
}

void leader(vector<uint8_t>& x, Node& node) {
    uint8_t leader = x[1];
    if(leader == 0) {
        node.changeLeader(true);
        node.changeSync(0);
        // Ustawiam czas wyslania pierwszego SYNC_START do wszystkich znanych wezlow na 2s, (5s - 3s = 2s).
        time5s = timeSinceStart() - 3000;   
    }
    else if(leader == 255 && node.getLeader()) {
        node.changeLeader(false);
        node.changeSyncNode();
        node.changeIgnoreSync(false);   // Węzeł przestaje wysyłać komunikaty synchronizujace do innych węzłów.
    }
    else {
        komerr(x, 2);
    }
}

void sendTime(vector<uint8_t>& x, Node& node, const struct sockaddr_in& peer_address, int socket) {
    fill(x.begin(), x.end(), 0);
    x[0] = 32;
    x[1] = node.getSync();
    uint64_t Time = timeSinceStart();

    if(node.getSyncNode().getToDate())     // Wysyłamy czas pomniejszony o offset, jeżeli węzeł jest zsynchronizowany.
        Time -= node.getOffset();

    parseTime(x, 2, Time);
    sendMessage(x, 10, peer_address, socket);
}

// Wektor 'temp' powienien zawierać adres jednego węzła, którego parametry dostaliśmy na wejściu.
struct sockaddr_in sendHello(Node& node, int socket) { 
    vector<uint8_t> pom(2);
    const vector <pair<uint16_t, struct sockaddr_in>>& temp = node.getPeers();

    pom[0] = 1;
    struct sockaddr_in tempSocket = temp[0].second;
    sendMessage(pom, 1, tempSocket, socket);
    node.eraseFirstPeer();  // Usuwam pierwszy węzeł z listy znanych węzłów, ponieważ nie otrzymano jeszcze HelloReply.
    return tempSocket;
}

void sendHelloReply(vector<uint8_t>& x, Node& node, int socket, const struct sockaddr_in& peer_address) { 
    fill(x.begin(), x.end(), 0);
    x[0] = 2;
    vector <pair<uint16_t, struct sockaddr_in>> temp = node.getPeers();
    int tempSize = temp.size();

    if(tempSize * 7 + 3 > BUFFER_SIZE) {
        return;
    }
    
    uint16_t count = temp.size();
    size_t it = 3;
    x[1] = ((count >> 8) & 0xFF);
    x[2] = count - (x[1] << 8);

    for (size_t i = 0; i < temp.size(); i++) {
        x[it] = 4;
        uint32_t ip_net = temp[i].second.sin_addr.s_addr;
        x[it + 1] = ((ip_net >> 24) & 0xFF);
        x[it + 2] = ((ip_net >> 16) & 0xFF);
        x[it + 3] = ((ip_net >> 8) & 0xFF);
        x[it + 4] = (ip_net & 0xFF);
        x[it + 5] = ((temp[i].second.sin_port >> 8) & 0xFF);    // Numer portu w .sin_port jest w formacie Big-Endian.
        x[it + 6] = (temp[i].second.sin_port & 0xFF);
        it += 7;
    }

    sendMessage(x, it, peer_address, socket);
}

void helloReply(vector<uint8_t>& x, Node& node, struct sockaddr_in& peerSocket, int socket) {  
    uint16_t count = (x[1] << 8) + x[2]; 

    uint16_t it = 0, i = 3;
    struct sockaddr_in peer_addr;
    struct sockaddr_in own_adr = node.getSockaddr();

    vector<struct sockaddr_in> tempPeers;

    size_t messSize = 3 + count;
    if(count != 0)
        messSize = 10;

    while(it < count) {
        uint8_t peer_address_length = x[i];
        i++;

        if(peer_address_length != 4) {
            komerr(x, messSize);
            return;
        }

        memset(&peer_addr, 0, sizeof(peer_addr));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = x[i + 3] + (x[i + 2] << 8) + (x[i + 1] << 16) + (x[i] << 24); 
        i += peer_address_length;

        uint16_t peer_port = (x[i] << 8) + x[i + 1];
        peer_addr.sin_port = peer_port;
        i += 2;

        if(peer_port == 0) {
            komerr(x, messSize);
            return;
        }

        if(peer_addr.sin_addr.s_addr == own_adr.sin_addr.s_addr && peer_addr.sin_port == own_adr.sin_port) {
            komerr(x, messSize);
            return;
        }

        if(peer_addr.sin_addr.s_addr == peerSocket.sin_addr.s_addr && peer_addr.sin_port == peerSocket.sin_port) {
            komerr(x, messSize);
            return;
        }

        tempPeers.push_back(peer_addr);
        it++;
    }

    // Jeżeli komunikat okazał się być poprawnym, wysyłam CONNECT do wszystkich węzłów.
    for(size_t i = 0; i < tempPeers.size(); i++) {
        vector<uint8_t> pom(2);
        pom[0] = 3;
        sendMessage(pom, 1, tempPeers[i], socket);
    }
}

void sendAckConnect(const struct sockaddr_in& peer_address, int socket) {  
    vector<uint8_t> pom(2);
    pom[0] = 4;
    sendMessage(pom, 1, peer_address, socket);
}

void sendDelayRequest(const struct sockaddr_in& peer_address, int socket) {
    vector<uint8_t> pom(2);
    pom[0] = 12;
    sendMessage(pom, 1, peer_address, socket);
}

void syncStart(vector<uint8_t>& x, Node& node, uint16_t peer_port, const struct sockaddr_in& peer_address, int socket) {
    SyncNode temp = node.getSyncNode();
    uint8_t peer_sync = x[1];
    uint64_t T1 = readTime(x, 2);

    if(!temp.isSyncNode(peer_port, peer_address)) {
        if(temp.getTrySync()) { // Inny node się już synchornizuje.
            komerr(x, 10);
            return;
        }
    }   // Sprawdzam, czy węzeł z którym jesteśmy zsynchronizowani, odpowiedział nam w przeciągu 30s. 
    else {
        uint64_t tempTime = timeSinceStart() - time20s;
        if(tempTime > 30000 || peer_sync >= node.getSync()) {
            node.changeSyncNode();
            komerr(x, 10);
            return;
        }
    }
    
    node.changeT(T1, 1);

    if(!node.knownPeer(peer_port, peer_address) || peer_sync >= 254) {
        komerr(x, 10);
        return;
    }

    if(!(temp.isSyncNode(peer_port, peer_address) &&  peer_sync < node.getSync())) {
        if(!(!temp.isSyncNode(peer_port, peer_address) && peer_sync + 2 <= node.getSync())) {
            komerr(x, 10);
            return;
        }
    }

    node.getSyncNode().setTrySync(true);    // Ustawiam ze próba synchornizacji.
    node.getSyncNode().setNode(peer_port, peer_address, peer_sync);

    uint64_t T2 = timeSinceStart();
    node.changeT(T2, 2);

    sendDelayRequest(peer_address, socket);

    uint64_t T3 = timeSinceStart();
    node.changeT(T3, 3);
}

void sendSyncStart(vector<uint8_t>& x, Node& node, const struct sockaddr_in& peer_address, int socket) {
    fill(x.begin(), x.end(), 0);
    x[0] = 11;
    x[1] = node.getSync();
    uint64_t T1 = timeSinceStart();

    if(node.getSyncNode().getToDate())     // Wysyłamy czas pomniejszony o offset z połaczonym węzłem.
        T1 -= node.getOffset();

    parseTime(x, 2, T1);
    sendMessage(x, 10, peer_address, socket);
}

void sendDelayResponse(vector<uint8_t>& x, Node& node, const struct sockaddr_in& peer_address, int socket) {
    fill(x.begin(), x.end(), 0);
    x[0] = 13;
    x[1] = node.getSync();
    uint64_t T4 = timeSinceStart();

    if(node.getSyncNode().getToDate())     // Wysyłamy czas pomniejszony o offset z połaczonym węzłem.
        T4 -= node.getOffset();

    parseTime(x, 2, T4);
    sendMessage(x, 10, peer_address, socket);
}

void delayResponse(vector<uint8_t>& x, Node& node) {
    uint8_t peer_sync = x[1];
    uint64_t T4 = readTime(x, 2);

    node.changeT(T4, 4);
    vector<uint64_t> T = node.getT();

    int offset = (T[2] - T[1] + T[3] - T[4]) / 2;

    SyncNode temp = node.getSyncNode();
    if(peer_sync == temp.getSync()) {       // Weryfikacja czy synchronizacja się powiodła.
        node.getSyncNode().setToDate(true);
        node.getSyncNode().setTrySync(false);
        node.changeOffset(offset);
        node.changeSync(peer_sync + 1);
        time20s = timeSinceStart();
    }
    else {  // Synchronacja się nie powiodła.
        komerr(x, 10);
        node.getSyncNode().setToDate(false);
        node.getSyncNode().setTrySync(false);
    }  
}

void sendSyncStartAll(vector<uint8_t>& x, Node& node, int socket) {
    const vector <pair<uint16_t, struct sockaddr_in>>& temp = node.getPeers();

    for(size_t i = 0; i < temp.size(); i++) {
        sendSyncStart(x, node, temp[i].second, socket);
    }
}

int main(int argc, char *argv[]) {
    bool syncReceived = true;
    time5s = timeSinceStart();
    Node nodeMain = entryInfo(argc, argv);
    
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }
    
    if (bind(socket_fd, (struct sockaddr *) &nodeMain.getSockaddr(), (socklen_t) sizeof(nodeMain.getSockaddr())) < 0) {
        syserr("binding failed");
    }

    struct sockaddr_in helloSocket;
    if(nodeMain.getHelloApproval())
        helloSocket = sendHello(nodeMain, socket_fd);

    vector <uint8_t> buffer(BUFFER_SIZE);

    ssize_t received_length;
    struct sockaddr_in client_address;
    socklen_t address_length = (socklen_t) sizeof(client_address);

    struct pollfd fds[1];
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    time9s = timeSinceStart();
    while(true) {

        uint64_t tempTime = timeSinceStart() - time5s;
        uint64_t tempTimeSync = timeSinceStart() - time20s;
        uint64_t tempTimeStop = timeSinceStart() - time9s;

        // Przez 25s nie dostaliśmy odpowiedzi od węzła, z którym jesteśmy zsynchornizowani.
        if(nodeMain.getSyncNode().getToDate() && tempTimeSync >= 25000)  {
            nodeMain.changeSyncNode();
        }

        if(!nodeMain.getIgnoreSync()) {
            if(tempTime > 5000 && nodeMain.getSync() < 254) {
                sendSyncStartAll(buffer, nodeMain, socket_fd);
                time5s = timeSinceStart();
                if(syncReceived && nodeMain.getPeers().size() != 0) {
                    time9s = timeSinceStart();
                    syncReceived = false;
                }
            }

            if(syncReceived) {
                time9s = timeSinceStart();
            }

            if(tempTimeStop > 9000) { // Po 9s nie dostaliśmy na nic odpowiedzi, zaprzestajemy prób synchronizacji.
                nodeMain.changeIgnoreSync(true); 
            }
        }

        fill(buffer.begin(), buffer.end(), 0);
        memset(&client_address, 0, sizeof(client_address));

        int poll_status = poll(fds, 1, 100); // 100 ms timeout

        if (poll_status < 0) {
            error("poll failed");
            continue;
        }

        if (poll_status == 0) {
            continue;
        }
        
        if (fds[0].revents & POLLIN) {
            received_length = recvfrom(socket_fd, buffer.data(), BUFFER_SIZE, 0,
                                        (struct sockaddr *) &client_address, &address_length);
            if (received_length < 0) {
                error("recvfrom");
                continue;
            }

            uint16_t client_port = ntohs(client_address.sin_port);

            switch(buffer[0]) {
                case 1:  // HELLO
                    sendHelloReply(buffer, nodeMain, socket_fd, client_address);
                    nodeMain.addPeer(client_port, client_address);                        
                    break;
                case 2: // HELLO_REPLY
                    if(nodeMain.getHelloApproval() && (helloSocket.sin_addr.s_addr == client_address.sin_addr.s_addr && helloSocket.sin_port == client_address.sin_port)) {
                        helloReply(buffer, nodeMain, client_address, socket_fd); 
                        nodeMain.addPeer(client_port, client_address);
                    }
                    else {
                        komerr(buffer, 3);
                    }   
                    break;
                case 3: // CONNECT
                    nodeMain.addPeer(client_port, client_address); 
                    sendAckConnect(client_address, socket_fd);
                    break;
                case 4: // ACK_CONNECT
                    nodeMain.addPeer(client_port, client_address);
                    break;
                case 11: // SYNC_START
                    if(!nodeMain.getIgnoreSync()) {   // wpp. ignorujemy
                        syncStart(buffer, nodeMain, client_port, client_address, socket_fd);
                    }
                    else {
                        komerr(buffer, 10);
                    }
                    break;
                case 12: // DELAY_REQUEST
                    if(!nodeMain.getIgnoreSync()) {
                        if(nodeMain.knownPeer(client_port, client_address)) {
                            syncReceived = true;
                            sendDelayResponse(buffer, nodeMain, client_address, socket_fd);
                        }
                        else {
                            komerr(buffer, 1);
                        }
                    }
                    else {
                        komerr(buffer, 1);
                    }
                    break;
                case 13: // DELAY_RESPONSE
                    if(!nodeMain.getIgnoreSync()) {
                        // Wiadomość ta jest tylko oczekiwana gdy podjeliśmy próbę sync z danym węzłem.
                        if(nodeMain.knownPeer(client_port, client_address) && nodeMain.getSyncNode().isSyncNode(client_port, client_address)) {
                            syncReceived = true;
                            delayResponse(buffer, nodeMain);
                        }
                        else {
                            komerr(buffer, 10);
                        }
                    }
                    else {
                        komerr(buffer, 10);
                    }
                    break;
                case 21: // LEADER
                    leader(buffer, nodeMain);
                    break;
                case 31: // GET_TIME
                    sendTime(buffer, nodeMain, client_address, socket_fd);
                    break;
                default:    // Nieznany komunikat.
                    komerr(buffer, 10);
                
            }
        }
    }

    close(socket_fd);
    return 0;
}
