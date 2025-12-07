#include "MatchingEngine.hpp"
#include <iostream>

int main()
{
    MatchingEngine engine;

    auto order1 = std::make_shared<Order>(1, 0, Side::BUY, OrderType::LIMIT, 100, 150.00);
    auto order2 = std::make_shared<Order>(2, 0, Side::BUY, OrderType::LIMIT, 200, 145.00);
    auto order3 = std::make_shared<Order>(3, 0, Side::BUY, OrderType::LIMIT, 200, 140.00);
    auto order4 = std::make_shared<Order>(4, 0, Side::BUY, OrderType::LIMIT, 200, 138.00);

    auto order5 = std::make_shared<Order>(5, 0, Side::SELL, OrderType::LIMIT, 150, 152.0);
    auto order6 = std::make_shared<Order>(6, 0, Side::SELL, OrderType::LIMIT, 100, 155.0);
    auto order7 = std::make_shared<Order>(7, 0, Side::SELL, OrderType::LIMIT, 100, 160.0);
    auto order8 = std::make_shared<Order>(8, 0, Side::SELL, OrderType::LIMIT, 100, 162.0);

    engine.SubmitOrder(order1);
    engine.SubmitOrder(order2);
    engine.SubmitOrder(order3);
    engine.SubmitOrder(order4);
    engine.SubmitOrder(order5);
    engine.SubmitOrder(order6);
    engine.SubmitOrder(order7);
    engine.SubmitOrder(order8);

    auto book = engine.GetBook(0);
    book->PrintBook();

    std::cout << "Cancelling order 2...\n";
    engine.CancelOrder(2);

    book->PrintBook();

    return 0;
}
