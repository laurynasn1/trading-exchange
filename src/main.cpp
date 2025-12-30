#include <iostream>
#include <memory>

#include "MatchingEngine.hpp"
#include "OrderGateway.hpp"
#include "MarketDataPublisher.hpp"

int main()
{
    const size_t NUM_ORDERS = 1000;
    const size_t QUEUE_SIZE = 1000;
    auto inputQueue = std::make_shared<SPSCQueue<OrderRequest>>(QUEUE_SIZE);
    auto outputQueue = std::make_shared<SPSCQueue<MarketDataEvent>>(QUEUE_SIZE);

    OrderGateway gateway(inputQueue, NUM_ORDERS);
    QueueOutputPolicy output(outputQueue);
    MatchingEngine<QueueOutputPolicy> engine(inputQueue, output, NUM_SYMBOLS, NUM_ORDERS);
    UDPTransmitter transmitter;
    MarketDataPublisher publisher(outputQueue, transmitter, NUM_ORDERS);

    publisher.Start();
    engine.Start();
    gateway.Start();

    gateway.WaitUntilFinished();
    while (!inputQueue->IsEmpty());
    engine.Stop();
    while (!outputQueue->IsEmpty());
    publisher.Stop();

    return 0;
}
