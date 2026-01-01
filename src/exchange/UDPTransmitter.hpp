#pragma once

#include <iostream>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "ItchMessage.hpp"
#include "Timer.hpp"

class UDPTransmitter
{
private:
    int sock;
    sockaddr_in groupSock;

    uint64_t nextSequenceNumber = 1;

    template<typename MsgType>
    void SendMsg(const MsgType & msg)
    {
        const char* buf = reinterpret_cast<const char*>(&msg);
        size_t bytesToSend = sizeof(msg);
        size_t sent = 0;
        while (sent < bytesToSend)
        {
            auto res = sendto(sock, buf + sent, bytesToSend, 0, (sockaddr*)&groupSock, sizeof(groupSock));
            if (res == -1)
            {
                perror("send");
                return;
            }
            sent += res;
        }
    }

    void MakeHeader(ItchHeader* header, char msgType, uint64_t timestamp)
    {
        header->sequenceNumber = htobe64(nextSequenceNumber++);
        header->messageType = msgType;
        header->timestamp = htobe64(timestamp);
    }

public:
    UDPTransmitter()
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1)
        {
            perror("socket");
            throw std::runtime_error{ "Failed to create socket" };
        }

        groupSock.sin_family = AF_INET;
        groupSock.sin_addr.s_addr = inet_addr("239.0.0.1");
        groupSock.sin_port = htons(12345);
        memset(groupSock.sin_zero, 0, sizeof(groupSock.sin_zero));

        in_addr localIface = {};
        localIface.s_addr = inet_addr("127.0.0.1");
        setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &localIface, sizeof(localIface));
    }

    ~UDPTransmitter()
    {
        std::cout << "UDP Transmitter sent " << nextSequenceNumber - 1 << " messages\n";
        close(sock);
    }

    void SendOrderAdd(uint64_t orderId, std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t timestamp)
    {
        OrderAddMsg msg;
        MakeHeader(&msg.header, 'A', timestamp);
        msg.orderId = htobe64(orderId);
        msg.side = side;
        msg.quantity = htobe32(quantity);
        memset(msg.symbol, 0, sizeof(msg.symbol));
        memcpy(msg.symbol, symbol.data(), std::min(symbol.length(), size_t(4)));
        msg.price = htobe32(price);
        SendMsg(msg);
    }

    void SendOrderExecuted(uint64_t orderId, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp)
    {
        OrderExecMsg msg;
        MakeHeader(&msg.header, 'E', timestamp);
        msg.orderId = htobe64(orderId);
        msg.quantity = htobe32(quantity);
        msg.matchId = htobe64(matchNumber);
        SendMsg(msg);
    }

    void SendOrderDeleted(uint64_t orderId, uint64_t timestamp)
    {
        OrderDeleteMsg msg;
        MakeHeader(&msg.header, 'D', timestamp);
        msg.orderId = htobe64(orderId);
        SendMsg(msg);
    }

    void SendTradeMessage(std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp)
    {
        TradeMsg msg;
        MakeHeader(&msg.header, 'P', timestamp);
        msg.side = side;
        msg.quantity = htobe32(quantity);
        memset(msg.symbol, 0, sizeof(msg.symbol));
        memcpy(msg.symbol, symbol.data(), std::min(symbol.length(), size_t(4)));
        msg.price = htobe32(price);
        msg.matchId = htobe64(matchNumber);
        SendMsg(msg);
    }

    void SendEndMarketHours()
    {
        EndMarketMsg msg;
        MakeHeader(&msg.header, 'M', Timer::rdtsc());
        SendMsg(msg);
    }
};

class NoOpTransmitter
{
public:
    void SendOrderAdd(uint64_t orderId, std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t timestamp) {}
    void SendOrderExecuted(uint64_t orderId, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp) {}
    void SendOrderDeleted(uint64_t orderId, uint64_t timestamp) {}
    void SendTradeMessage(std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp) {}
    void SendEndMarketHours() {}
};