#include "xquotes_parameter_array_storage.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;

    xquotes_parameter_array_storage::ParameterArrays iParameterArrays;
    xquotes_daily_data_storage::DailyDataStorage<xquotes_parameter_array_storage::Parameter2D<float,2>> iParameterStorage("test.dat");

    xquotes_parameter_array_storage::Parameter2D<float,2> iParameter2D;
    iParameter2D.set_rows(3);

    iParameter2D.set(1.0f, 0, 0);
    iParameter2D.set(2.0f, 1, 0);
    iParameter2D.set(3.0f, 0, 1);
    iParameter2D.set(4.0f, 1, 1);
    iParameterStorage.write_day_data(iParameter2D, xtime::get_timestamp(1,1,2018));

    iParameter2D.set(9.0f, 0, 0);
    iParameter2D.set(1.0f, 1, 0);
    iParameter2D.set(9.0f, 0, 1);
    iParameter2D.set(2.0f, 1, 1);
    iParameterStorage.write_day_data(iParameter2D, xtime::get_timestamp(2,1,2018));

    iParameter2D.set(11.0f, 0, 0);
    iParameter2D.set(11.0f, 1, 0);
    iParameter2D.set(12.0f, 0, 1);
    iParameter2D.set(22.0f, 1, 1);
    iParameterStorage.write_day_data(iParameter2D, xtime::get_timestamp(3,1,2018));

    xquotes_parameter_array_storage::Parameter2D<float,2> iParameter2DTemp;
    //iParameter2DTemp.set_rows(3);

    iParameterStorage.get_day_data(iParameter2DTemp, xtime::get_timestamp(2,1,2018));
    std::cout << "get " << iParameter2DTemp.get(0,1) << std::endl;
    std::cout << "get " << iParameter2DTemp.get(1,1) << std::endl;
    std::cout << "get_rows " << iParameter2DTemp.get_rows() << std::endl;
    system("pause");
    return 0;
}


