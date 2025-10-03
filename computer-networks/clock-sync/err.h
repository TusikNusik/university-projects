#ifndef MIM_ERR_H
#define MIM_ERR_H

void syserr(const std::string& x);

void komerr(std::vector<uint8_t>& x, size_t rozmiar);

void error(const std::string& x);

#endif