#include <iostream>
#include "xquotes_dictionary.hpp"

using namespace std;

int main()
{
    cout << "start!" << endl;
    for(int i = 0; i < sizeof(xquotes_dictionary::candles); ++i) {
        cout << (int)xquotes_dictionary::candles[i] << endl;
    }
    return 0;
}
