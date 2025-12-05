#include <gtest/gtest.h>
#include <MatchingEngine.hpp>
#include <thread>
#include <chrono>

class MatchingEngineTest : public testing::Test
{
public:
    std::vector<MarketDataEvent> events;
    MatchingEngine engine;

    MatchingEngineTest() :
        engine([&](const auto & event) {
            events.push_back(event);
        }) {}
};

TEST_F(MatchingEngineTest, BasicLimitOrderMatching)
{
    auto sell = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00);
    engine.SubmitOrder(sell);
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[0].quantity, 100);
    EXPECT_EQ(events[0].price, 150.00);

    auto buy = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 100, 150.00);
    engine.SubmitOrder(buy);
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[1].restingOrderId, 1);
    EXPECT_EQ(events[1].quantity, 100);
    EXPECT_EQ(events[1].price, 150.00);
}

TEST_F(MatchingEngineTest, PartialFill)
{
    auto sell = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 200, 150.00);
    engine.SubmitOrder(sell);

    auto buy = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 100, 150.00);
    engine.SubmitOrder(buy);

    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[0].quantity, 200);
    EXPECT_EQ(events[0].price, 150.00);

    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[1].restingOrderId, 1);
    EXPECT_EQ(events[1].quantity, 100);
    EXPECT_EQ(events[1].price, 150.00);

    auto book = engine.GetBook("AAPL");
    auto [bid, ask] = book->GetTopOfBook();
    EXPECT_EQ(ask, 150.00);
}

TEST_F(MatchingEngineTest, PriceTimePriority)
{
    auto sell1 = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00);
    auto sell2 = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00);

    engine.SubmitOrder(sell1);
    engine.SubmitOrder(sell2);

    auto buy = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::LIMIT, 50, 150.00);
    engine.SubmitOrder(buy);

    EXPECT_EQ(events.size(), 3);

    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(events[2].orderId, 3);
    EXPECT_EQ(events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[2].restingOrderId, 1);
}

TEST_F(MatchingEngineTest, MarketOrder)
{
    engine.SubmitOrder(std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00));
    engine.SubmitOrder(std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.05));

    auto market = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::MARKET, 150, 0.0);
    engine.SubmitOrder(market);

    EXPECT_EQ(events.size(), 4);

    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(events[2].orderId, 3);
    EXPECT_EQ(events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[2].restingOrderId, 1);
    EXPECT_EQ(events[2].quantity, 100);

    EXPECT_EQ(events[3].orderId, 3);
    EXPECT_EQ(events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[3].restingOrderId, 2);
    EXPECT_EQ(events[3].quantity, 50);
}

TEST_F(MatchingEngineTest, IOCOrder)
{
    engine.SubmitOrder(std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00));
    engine.SubmitOrder(std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.05));
    engine.SubmitOrder(std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.10));

    auto market = std::make_shared<Order>(4, "AAPL", Side::BUY, OrderType::IOC, 250, 150.05);
    engine.SubmitOrder(market);

    EXPECT_EQ(events.size(), 5);

    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[2].orderId, 3);
    EXPECT_EQ(events[2].type, EventType::ORDER_ACKED);

    EXPECT_EQ(events[3].orderId, 4);
    EXPECT_EQ(events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[3].restingOrderId, 1);
    EXPECT_EQ(events[3].quantity, 100);

    EXPECT_EQ(events[4].orderId, 4);
    EXPECT_EQ(events[4].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[4].restingOrderId, 2);
    EXPECT_EQ(events[4].quantity, 100);

    auto book = engine.GetBook("AAPL");
    auto [bid, ask] = book->GetTopOfBook();
    EXPECT_EQ(ask, 150.10);
}

TEST_F(MatchingEngineTest, FOKOrderAccepted)
{
    engine.SubmitOrder(std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00));
    engine.SubmitOrder(std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.05));

    auto market = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::FOK, 200, 0.0);
    engine.SubmitOrder(market);

    EXPECT_EQ(events.size(), 4);

    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(events[2].orderId, 3);
    EXPECT_EQ(events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[2].restingOrderId, 1);
    EXPECT_EQ(events[2].quantity, 100);

    EXPECT_EQ(events[3].orderId, 3);
    EXPECT_EQ(events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(events[3].restingOrderId, 2);
    EXPECT_EQ(events[3].quantity, 100);
}

TEST_F(MatchingEngineTest, FOKOrderRejected)
{
    engine.SubmitOrder(std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.00));
    engine.SubmitOrder(std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100, 150.05));

    auto market = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::FOK, 201, 0.0);
    engine.SubmitOrder(market);

    EXPECT_EQ(events.size(), 3);
    EXPECT_EQ(events[0].orderId, 1);
    EXPECT_EQ(events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[1].orderId, 2);
    EXPECT_EQ(events[1].type, EventType::ORDER_ACKED);
    EXPECT_EQ(events[2].orderId, 3);
    EXPECT_EQ(events[2].type, EventType::ORDER_REJECTED);
}