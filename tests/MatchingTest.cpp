#include <gtest/gtest.h>
#include "MatchingEngine.hpp"
#include <thread>
#include <chrono>

class MatchingEngineTest : public testing::Test
{
public:
    VectorOutputPolicy output;
    MatchingEngine<VectorOutputPolicy> engine;

    MatchingEngineTest() :
        engine(output) {}
};

TEST_F(MatchingEngineTest, BasicLimitOrderMatching)
{
    Order sell{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    engine.SubmitOrder(&sell);
    EXPECT_EQ(output.events.size(), 1);
    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[0].quantity, 100);
    EXPECT_EQ(output.events[0].price, 15000);

    Order buy{ 2, 0, Side::BUY, OrderType::LIMIT, 100, 15000 };
    engine.SubmitOrder(&buy);
    EXPECT_EQ(output.events.size(), 2);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[1].restingOrderId, 1);
    EXPECT_EQ(output.events[1].quantity, 100);
    EXPECT_EQ(output.events[1].price, 15000);
}

TEST_F(MatchingEngineTest, PartialFill)
{
    Order sell{ 1, 0, Side::SELL, OrderType::LIMIT, 200, 15000 };
    engine.SubmitOrder(&sell);

    Order buy{ 2, 0, Side::BUY, OrderType::LIMIT, 100, 15000 };
    engine.SubmitOrder(&buy);

    EXPECT_EQ(output.events.size(), 2);
    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[0].quantity, 200);
    EXPECT_EQ(output.events[0].price, 15000);

    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[1].restingOrderId, 1);
    EXPECT_EQ(output.events[1].quantity, 100);
    EXPECT_EQ(output.events[1].price, 15000);

    auto book = engine.GetBook(0);
    auto [bid, ask] = book->GetTopOfBook();
    EXPECT_EQ(ask, 15000);
}

TEST_F(MatchingEngineTest, PriceTimePriority)
{
    Order sell1{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    Order sell2{ 2, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };

    engine.SubmitOrder(&sell1);
    engine.SubmitOrder(&sell2);

    Order buy{ 3, 0, Side::BUY, OrderType::LIMIT, 50, 15000 };
    engine.SubmitOrder(&buy);

    EXPECT_EQ(output.events.size(), 3);

    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(output.events[2].orderId, 3);
    EXPECT_EQ(output.events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[2].restingOrderId, 1);
}

TEST_F(MatchingEngineTest, MarketOrder)
{
    Order sell1{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    Order sell2{ 2, 0, Side::SELL, OrderType::LIMIT, 100, 15005 };

    engine.SubmitOrder(&sell1);
    engine.SubmitOrder(&sell2);

    Order market{ 3, 0, Side::BUY, OrderType::MARKET, 150, 0 };
    engine.SubmitOrder(&market);

    EXPECT_EQ(output.events.size(), 4);

    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(output.events[2].orderId, 3);
    EXPECT_EQ(output.events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[2].restingOrderId, 1);
    EXPECT_EQ(output.events[2].quantity, 100);

    EXPECT_EQ(output.events[3].orderId, 3);
    EXPECT_EQ(output.events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[3].restingOrderId, 2);
    EXPECT_EQ(output.events[3].quantity, 50);
}

TEST_F(MatchingEngineTest, IOCOrder)
{
    Order sell1{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    Order sell2{ 2, 0, Side::SELL, OrderType::LIMIT, 100, 15005 };
    Order sell3{ 3, 0, Side::SELL, OrderType::LIMIT, 100, 15010 };

    engine.SubmitOrder(&sell1);
    engine.SubmitOrder(&sell2);
    engine.SubmitOrder(&sell3);

    Order market{ 4, 0, Side::BUY, OrderType::IOC, 250, 15005 };
    engine.SubmitOrder(&market);

    EXPECT_EQ(output.events.size(), 6);

    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[2].orderId, 3);
    EXPECT_EQ(output.events[2].type, EventType::ORDER_ACKED);

    EXPECT_EQ(output.events[3].orderId, 4);
    EXPECT_EQ(output.events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[3].restingOrderId, 1);
    EXPECT_EQ(output.events[3].quantity, 100);

    EXPECT_EQ(output.events[4].orderId, 4);
    EXPECT_EQ(output.events[4].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[4].restingOrderId, 2);
    EXPECT_EQ(output.events[4].quantity, 100);

    EXPECT_EQ(output.events[5].orderId, 4);
    EXPECT_EQ(output.events[5].type, EventType::ORDER_CANCELLED);

    auto book = engine.GetBook(0);
    auto [bid, ask] = book->GetTopOfBook();
    EXPECT_EQ(ask, 15010);
}

TEST_F(MatchingEngineTest, FOKOrderAccepted)
{
    Order sell1{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    Order sell2{ 2, 0, Side::SELL, OrderType::LIMIT, 100, 15005 };

    engine.SubmitOrder(&sell1);
    engine.SubmitOrder(&sell2);

    Order market{ 3, 0, Side::BUY, OrderType::FOK, 200, 0 };
    engine.SubmitOrder(&market);

    EXPECT_EQ(output.events.size(), 4);

    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_ACKED);

    EXPECT_EQ(output.events[2].orderId, 3);
    EXPECT_EQ(output.events[2].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[2].restingOrderId, 1);
    EXPECT_EQ(output.events[2].quantity, 100);

    EXPECT_EQ(output.events[3].orderId, 3);
    EXPECT_EQ(output.events[3].type, EventType::ORDER_FILLED);
    EXPECT_EQ(output.events[3].restingOrderId, 2);
    EXPECT_EQ(output.events[3].quantity, 100);
}

TEST_F(MatchingEngineTest, FOKOrderRejected)
{
    Order sell1{ 1, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
    Order sell2{ 2, 0, Side::SELL, OrderType::LIMIT, 100, 15005 };

    engine.SubmitOrder(&sell1);
    engine.SubmitOrder(&sell2);

    Order market{ 3, 0, Side::BUY, OrderType::FOK, 201, 0 };
    engine.SubmitOrder(&market);

    EXPECT_EQ(output.events.size(), 3);
    EXPECT_EQ(output.events[0].orderId, 1);
    EXPECT_EQ(output.events[0].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[1].orderId, 2);
    EXPECT_EQ(output.events[1].type, EventType::ORDER_ACKED);
    EXPECT_EQ(output.events[2].orderId, 3);
    EXPECT_EQ(output.events[2].type, EventType::ORDER_CANCELLED);
}