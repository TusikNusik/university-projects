#ifndef MIM_COMMON_H
#define MIM_COMMON_H

namespace detail {
    uint16_t readPort(char const *string);

    unsigned readNumber(char const *string);

    std::vector<double> readCoeffs(std::string x, unsigned N);

    std::pair<bool, size_t> checkForEndline(std::vector<char>& x);

    unsigned countSmallLetters(const std::string& playerID);

    std::string formatDouble(double value);

    std::string formatClientAddress(const sockaddr_storage& addr);
};

// Musiałem umieścić tę funkcję poza namespace, ponieważ dostawałem błędy związane z templetami.
template <typename T> 
void dataAppearedValidation(std::vector<bool>& x, size_t index, T& data, T dataToCopy);

#endif
