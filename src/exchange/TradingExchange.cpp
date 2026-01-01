#include <iostream>
#include <memory>

#include "MatchingEngine.hpp"
#include "OrderGateway.hpp"
#include "MarketDataPublisher.hpp"

int main(int argc, char **argv)
{
    const size_t DEFAULT_QUEUE_SIZE = 1000;

    int numOrders;
    int numSymbols = MAX_NUM_SYMBOLS;
    int queueSize = DEFAULT_QUEUE_SIZE;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <num_orders> [<num_symbols> (default " << MAX_NUM_SYMBOLS << ")] [<queue_size> (default " << DEFAULT_QUEUE_SIZE << ")]\n";
        return 1;
    }

    numOrders = atoi(argv[1]);

    if (numOrders <= 0)
    {
        std::cout << "num_orders must be greater than 0\n";
        return 1;
    }

    if (argc >= 3)
    {
        numSymbols = atoi(argv[2]);
        if (numSymbols <= 0 || numSymbols > MAX_NUM_SYMBOLS)
        {
            std::cout << "num_symbols must be between 1 and " << MAX_NUM_SYMBOLS << "\n";
            return 1;
        }
    }

    if (argc >= 4)
    {
        queueSize = atoi(argv[3]);
        if (queueSize <= 1)
        {
            std::cout << "queue_size must be greater than 1\n";
            return 1;
        }
    }

    auto inputQueue = std::make_shared<SPSCQueue<OrderRequest>>(queueSize);
    auto outputQueue = std::make_shared<SPSCQueue<MarketDataEvent>>(queueSize);

    OrderGateway gateway(inputQueue, numSymbols, numOrders);
    QueueOutputPolicy output(outputQueue);
    MatchingEngine<QueueOutputPolicy> engine(inputQueue, output, numSymbols, numOrders);
    UDPTransmitter transmitter;
    MarketDataPublisher publisher(outputQueue, transmitter, numOrders);

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
