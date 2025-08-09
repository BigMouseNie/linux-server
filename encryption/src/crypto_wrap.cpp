#include "crypto_wrap.h"

#include <openssl/md5.h>

#include <iomanip>
#include <iostream>
#include <sstream>

std::string CryptoWrap::GenMD5(const std::string& input) {
  unsigned char digest[MD5_DIGEST_LENGTH];
  MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(),
      digest);

  std::ostringstream ss;
  for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
  }

  return ss.str();
}
