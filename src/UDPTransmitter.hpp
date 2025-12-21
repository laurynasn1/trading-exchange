#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstring>

#pragma pack(push, 1)

struct ItchHeader
{
    char messageType;
    uint64_t timestamp;
};

struct OrderAddMsg
{
    ItchHeader header;
    uint64_t orderId;
    uint8_t side;
    uint32_t quantity;
    char symbol[4];
    uint32_t price;
};

struct OrderExecMsg
{
    ItchHeader header;
    uint64_t orderId;
    uint32_t quantity;
    uint64_t matchId;
};

struct OrderDeleteMsg
{
    ItchHeader header;
    u_int64_t orderId;
};

struct TradeMsg
{
    ItchHeader header;
    uint8_t side;
    uint32_t quantity;
    char symbol[4];
    uint32_t price;
    uint64_t matchId;
};

#pragma pack(pop)

class UDPTransmitter
{
private:
    int sock;
    sockaddr_in groupSock;

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

public:
    UDPTransmitter()
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1)
        {
            perror("sock");
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
        close(sock);
    }

    void SendOrderAdd(uint64_t orderId, std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t timestamp)
    {
        OrderAddMsg msg;
        msg.header.messageType = 'A';
        msg.header.timestamp = htobe64(timestamp);
        msg.orderId = htobe64(orderId);
        msg.side = side;
        msg.quantity = htobe32(quantity);
        memcpy(msg.symbol, symbol.data(), std::min(symbol.length(), size_t(4)));
        msg.price = htobe32(price);
        SendMsg(msg);
    }

    void SendOrderExecuted(uint64_t orderId, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp)
    {
        OrderExecMsg msg;
        msg.header.messageType = 'E';
        msg.header.timestamp = htobe64(timestamp);
        msg.orderId = htobe64(orderId);
        msg.quantity = htobe32(quantity);
        msg.matchId = htobe64(matchNumber);
        SendMsg(msg);
    }

    void SendOrderDeleted(uint64_t orderId, uint64_t timestamp)
    {
        OrderDeleteMsg msg;
        msg.header.messageType = 'D';
        msg.header.timestamp = htobe64(timestamp);
        msg.orderId = htobe64(orderId);
        SendMsg(msg);
    }

    void SendTradeMessage(std::string_view symbol, uint8_t side, uint32_t price, uint32_t quantity, uint64_t matchNumber, uint64_t timestamp)
    {
        TradeMsg msg;
        msg.header.messageType = 'P';
        msg.header.timestamp = htobe64(timestamp);
        msg.side = side;
        msg.quantity = htobe32(quantity);
        memcpy(msg.symbol, symbol.data(), std::min(symbol.length(), size_t(4)));
        msg.price = htobe32(price);
        msg.matchId = htobe64(matchNumber);
        SendMsg(msg);
    }
};