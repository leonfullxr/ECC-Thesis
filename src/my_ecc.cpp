// my_ecc.cpp
#include <iostream>
#include <NTL/ZZ.h>

using namespace std;
using namespace NTL;

int main() {
    ZZ a = conv<ZZ>(1);
    // Use straight quotes " not “ or ”
    cout << "NTL works: \"1\"+\"1\"=" << (a + a) << "\n";
    return 0;
}
