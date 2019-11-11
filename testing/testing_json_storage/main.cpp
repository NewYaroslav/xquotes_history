#include "xquotes_json_storage.hpp"
#include <iostream>

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;
    xquotes_json_storage::JsonStorage iStorage("test.dat");
    json j1;
    j1["test"] = 1;
    j1["str"] = "Hello!";

    json j2;
    j2["test"] = 2;
    j2["str"] = "Hey!";

    json j3;
    j3["test"] = 3;
    j3["str"] = "123";

    iStorage.write_json(j1, xtime::get_timestamp(1,1,2018));
    iStorage.write_json(j2, xtime::get_timestamp(2,1,2018));
    iStorage.write_json(j3, xtime::get_timestamp(2,1,2018));
    json temp;
    iStorage.get_json(temp, xtime::get_timestamp(2,1,2018));
    std::cout << "get_day_data " << temp << std::endl;
    std::cout << "get_day_data " << iStorage.get_json(xtime::get_timestamp(1,1,2018)) << std::endl;
    system("pause");
    return 0;
}


