#include "xquotes_daily_data_storage.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;
    xquotes_daily_data_storage::DailyDataStorage<std::string> iStorage("test.dat");
    iStorage.write_day_data("Hello!", xtime::get_timestamp(1,1,2018));
    iStorage.write_day_data("Hey!", xtime::get_timestamp(2,1,2018));
    iStorage.write_day_data("123", xtime::get_timestamp(2,1,2018));
    std::string temp;
    iStorage.get_day_data(temp, xtime::get_timestamp(2,1,2018));
    std::cout << "get_day_data " << temp << std::endl;
    std::cout << "get_day_data " << iStorage.get_day_data(xtime::get_timestamp(2,1,2018)) << std::endl;
    system("pause");
    return 0;
}


