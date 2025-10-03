#ifndef LOG_H
#define LOG_H

namespace logInfo {

    void printTime(uint64_t time);

    void printConnected(sockaddr_storage& addr, socklen_t addr_len, uint64_t time);

    void printPutValue(unsigned point, double value, uint64_t time);
    
    void printStateReceived(const std::vector <double>& states, uint64_t time);

    void printCoeffsReceived(const std::vector <double>& states, uint64_t time);

    void printBadPutReceived(unsigned point, double value, uint64_t time);

    void printPenaltyReceived(unsigned point, double value, uint64_t time);

    void printScoringReceived(const std::string& mess, uint64_t time);
    // ----------------------------SERWER COMMS--------------------------------

    void printOldNewClient(const std::string& IP, uint16_t port, uint64_t time);
    
    void printNewClient(const std::string& IP, uint64_t time);

    void printKnownClient(int fd, const std::string& name, uint64_t time);

    void printCoeffsSend(const std::vector <double>& coeffs, const std::string& name, uint64_t time);

    void printPutSend(const std::string& name, unsigned point, double value, uint64_t time);

    void printStateSend(const std::string& mess, const std::string& name, uint64_t time);

    void printScoringSend(const std::string& mess, uint64_t time);
}

#endif