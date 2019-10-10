#include "xquotes_container.hpp"


int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;
    xquotes_container::Container<xquotes_common::Candle> CandleContainer;

    xquotes_common::Candle candle(1.1,1.1,1.1,1.1,xtime::get_timestamp(1,1,2012));
    CandleContainer.add(candle);
    candle = xquotes_common::Candle(1.2,1.2,1.2,1.2,xtime::get_timestamp(2,1,2012));
    CandleContainer.add(candle);
    candle = xquotes_common::Candle(1.3,1.3,1.3,1.3,xtime::get_timestamp(3,1,2012));
    CandleContainer.add(candle);
    candle = xquotes_common::Candle(1.4,1.4,1.4,1.4,xtime::get_timestamp(4,1,2012));
    CandleContainer.add(candle);

    xquotes_common::Candle rcandle;
    int err = CandleContainer.get(rcandle, xtime::get_timestamp(3,1,2012));
    std::cout << xtime::get_str_date(rcandle.timestamp) << " candle c: " << rcandle.close << " err " << err << std::endl;

    CandleContainer.get(rcandle, xtime::get_timestamp(1,1,2012));
    std::cout << xtime::get_str_date(rcandle.timestamp) << " candle c: " << rcandle.close << " err " << err << std::endl;

    candle = xquotes_common::Candle(1.5,1.5,1.5,1.5,xtime::get_timestamp(5,1,2012));
    CandleContainer.add_with_del_old(candle, 3);

    err = CandleContainer.get(rcandle, xtime::get_timestamp(2,1,2012));
    std::cout << xtime::get_str_date(rcandle.timestamp) << " candle c: " << rcandle.close << " err " << err << std::endl;

    err = CandleContainer.get(rcandle, xtime::get_timestamp(3,1,2012));
    std::cout << xtime::get_str_date(rcandle.timestamp) << " candle c: " << rcandle.close << " err " << err << std::endl;

    err = CandleContainer.get(rcandle, xtime::get_timestamp(5,1,2012));
    std::cout << xtime::get_str_date(rcandle.timestamp) << " candle c: " << rcandle.close << " err " << err << std::endl;
    system("pause");
    return 0;
}


