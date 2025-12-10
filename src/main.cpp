#include "MatchingEngine.hpp"
#include <iostream>

int main()
{
    MatchingEngine engine;

    Order order1{ 1, 0, Side::BUY, OrderType::LIMIT, 100, 15000 };
    Order order2{ 2, 0, Side::BUY, OrderType::LIMIT, 200, 14500 };
    Order order3{ 3, 0, Side::BUY, OrderType::LIMIT, 200, 14000 };
    Order order4{ 4, 0, Side::BUY, OrderType::LIMIT, 200, 13800 };

    Order order5{ 5, 0, Side::SELL, OrderType::LIMIT, 150, 15200 };
    Order order6{ 6, 0, Side::SELL, OrderType::LIMIT, 100, 15500 };
    Order order7{ 7, 0, Side::SELL, OrderType::LIMIT, 100, 16000 };
    Order order8{ 8, 0, Side::SELL, OrderType::LIMIT, 100, 16200 };

    engine.SubmitOrder(&order1);
    engine.SubmitOrder(&order2);
    engine.SubmitOrder(&order3);
    engine.SubmitOrder(&order4);
    engine.SubmitOrder(&order5);
    engine.SubmitOrder(&order6);
    engine.SubmitOrder(&order7);
    engine.SubmitOrder(&order8);

    auto book = engine.GetBook(0);
    book->PrintBook();

    std::cout << "Cancelling order 2...\n";
    engine.CancelOrder(2);

    book->PrintBook();

    return 0;
}
