// common.hpp
#ifndef CRYPTO_COMMON_HPP
#define CRYPTO_COMMON_HPP

#include <NTL/ZZ.h>

namespace crypto {
    // Alias para enteros de precisión arbitraria usando NTL
    using BigInt = NTL::ZZ;
}

#endif // CRYPTO_COMMON_HPP