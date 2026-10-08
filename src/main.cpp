// Временная точка входа. Полноценный консольный UI появится в блоке 9.
#include <iostream>

#include "LR2/mutableArraySequence.h"
#include "LR2/sequence_print.h"

int main() {
    MutableArraySequence<int> seq;
    for (int i = 1; i <= 5; i++)
        seq.append(i * 10);

    std::cout << "Project skeleton is OK: " << seq << "\n";
    return 0;
}
