#include <iostream>
#include <NTL/ZZ.h>

using namespace std;
using namespace NTL;

int main() {
    // 1. Generar dos primos grandes p, q de 512 bits
    ZZ p = GenPrime_ZZ(512);   // genera un probable primo de 512 bits
    ZZ q = GenPrime_ZZ(512);
    ZZ n = p * q;              // módulo RSA
    ZZ phi = (p-1) * (q-1);    // phi(n)
    // 2. Elegir exponente público e, típico 65537
    ZZ e = ZZ(65537);
    // 3. Calcular exponente privado d = e^{-1} mod phi
    ZZ d = InvMod(e, phi);
    // (d existe si e es coprimo con phi, asegurado si e=65537 y p,q distintos)
    std::cout << "Clave pública: (n=" << n << ", e=" << e << ")\n";
    std::cout << "Clave privada: (d=" << d << ")\n";
    // 4. Ejemplo de cifrado/descifrado
    ZZ m = conv<ZZ>("12345678901234567890");  // mensaje numérico
    ZZ c = PowerMod(m, e, n);    // c = m^e mod n (cifrado)
    ZZ m2 = PowerMod(c, d, n);   // m2 = c^d mod n (descifrado)
    std::cout << "Mensaje original: " << m << "\n";
    std::cout << "Mensaje descifrado: " << m2 << "\n";
}
