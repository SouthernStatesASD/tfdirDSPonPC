#include <iostream>
#include "PhasorContainerV2.h"
#include "TFDIRV2.h"

int main() {
    std::cout << "Test TFDIR" << std::endl;

    PhasorContainerV2 *testPhasorContainer = new PhasorContainerV2();
    TFDIRV2 *testTFDIR = new TFDIRV2(testPhasorContainer);

    testTFDIR->testSTC();

    return 0;
}