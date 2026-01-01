#include <iostream>
#include <string>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ItchMessage.hpp"

constexpr size_t MAX_PACKET_SIZE = 128;

void HandleOrderAdd(OrderAddMsg* msg)
{
    uint64_t seqNumber = be64toh(msg->header.sequenceNumber);
    uint64_t timestamp = be64toh(msg->header.timestamp);
    uint64_t orderId = be64toh(msg->orderId);
    char side = msg->side;
    uint32_t quantity = be32toh(msg->quantity);
    std::string symbol(msg->symbol);
    uint32_t price = be32toh(msg->price);

    std::cout << "Order added: ID=" << orderId << " Side=" << side << " Symbol=" << symbol << " Quantity=" << quantity << " Price=$" << price / 100.0 << "\n";
}

void HandleOrderExecuted(OrderExecMsg* msg)
{
    uint64_t seqNumber = be64toh(msg->header.sequenceNumber);
    uint64_t timestamp = be64toh(msg->header.timestamp);
    uint64_t orderId = be64toh(msg->orderId);
    uint32_t quantity = be32toh(msg->quantity);
    uint64_t matchId = be64toh(msg->matchId);

    std::cout << "Order executed: ID=" << orderId << " Quantity=" << quantity << " MatchNumber=" << matchId << "\n";
}

void HandleOrderDeleted(OrderDeleteMsg* msg)
{
    uint64_t seqNumber = be64toh(msg->header.sequenceNumber);
    uint64_t timestamp = be64toh(msg->header.timestamp);
    uint64_t orderId = be64toh(msg->orderId);

    std::cout << "Order deleted: ID=" << orderId << "\n";
}

void HandleTradeMessage(TradeMsg* msg)
{
    uint64_t seqNumber = be64toh(msg->header.sequenceNumber);
    uint64_t timestamp = be64toh(msg->header.timestamp);
    char side = msg->side;
    uint32_t quantity = be32toh(msg->quantity);
    std::string symbol(msg->symbol);
    uint32_t price = be32toh(msg->price);
    uint64_t matchId = be64toh(msg->matchId);

    std::cout << "Trade message: Side=" << side << " Symbol=" << symbol << " Quantity=" << quantity << " Price=$" << price / 100.0 << " MatchNumber=" << matchId << "\n";
}

void ProcessMessages(int sock)
{
    char buf[MAX_PACKET_SIZE];
    uint64_t seqNumber = 1;
    uint64_t messagesDropped = 0;
    while (true)
    {
        int bytesRead = recvfrom(sock, buf, sizeof(buf), 0, nullptr, nullptr);
        ItchHeader* header = reinterpret_cast<ItchHeader*>(buf);

        uint64_t headerSeqNumber = be64toh(header->sequenceNumber);
        if (headerSeqNumber != seqNumber)
        {
            std::cout << "Sequence number mismatch: got " << headerSeqNumber << ", expected " << seqNumber << "\n";
            messagesDropped += headerSeqNumber - seqNumber;
        }

        switch (header->messageType)
        {
        case 'A':
            HandleOrderAdd(reinterpret_cast<OrderAddMsg*>(buf));
            break;
        case 'E':
            HandleOrderExecuted(reinterpret_cast<OrderExecMsg*>(buf));
            break;
        case 'D':
            HandleOrderDeleted(reinterpret_cast<OrderDeleteMsg*>(buf));
            break;
        case 'P':
            HandleTradeMessage(reinterpret_cast<TradeMsg*>(buf));
            break;
        case 'M':
            std::cout << "Messages processed: " << headerSeqNumber - messagesDropped << " Messages dropped: " << messagesDropped << "\n";
            return;
        default:
            break;
        }

        seqNumber = headerSeqNumber + 1;
    }
}

int main(int argc, char **argv)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    int rcvbuf = 4 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        return 1;
    }

    ip_mreq group = {};
    group.imr_multiaddr.s_addr = inet_addr("239.0.0.1");
    group.imr_interface.s_addr = inet_addr("127.0.0.1");
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&group, sizeof(group));

    ProcessMessages(sock);

    close(sock);
    return 0;
}
