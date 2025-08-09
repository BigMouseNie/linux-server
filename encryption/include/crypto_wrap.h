#ifndef ENCRYPATION_CRYPTOWRAP_H_
#define ENCRYPATION_CRYPTOWRAP_H_

#include <string>

class CryptoWrap {
public:
    static std::string GenMD5(const std::string& input);
private:
};

#endif  // ENCRYPATION_CRYPTOWRAP_H_
