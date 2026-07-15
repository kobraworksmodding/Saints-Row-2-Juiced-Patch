#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

constexpr const char* STUN_SERVER_1 = "stun.l.google.com";
constexpr const char* STUN_SERVER_2 = "stun1.l.google.com";
constexpr const char* STUN_PORT = "19302";

constexpr uint16_t STUN_BINDING_REQUEST = 0x0001;
constexpr uint16_t STUN_HEADER_LENGTH = 20;
constexpr uint32_t STUN_MAGIC_COOKIE = 0x2112A442;

std::string getLocalIp(SOCKET sock)
{
    sockaddr_in localAddr{};
    int addrLen = sizeof(localAddr);
    if (getsockname(sock, (sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR)
    {
        return "";
    }
    char ipStr[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &(localAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    return std::string(ipStr);
}

void createStunRequest(uint8_t* buffer)
{
    // STUN Message Type: Binding Request (0x0001)
    buffer[0] = (STUN_BINDING_REQUEST >> 8) & 0xFF;
    buffer[1] = STUN_BINDING_REQUEST & 0xFF;

    // Message Length: 0 (no attributes)
    buffer[2] = 0;
    buffer[3] = 0;

    // Magic Cookie
    buffer[4] = (STUN_MAGIC_COOKIE >> 24) & 0xFF;
    buffer[5] = (STUN_MAGIC_COOKIE >> 16) & 0xFF;
    buffer[6] = (STUN_MAGIC_COOKIE >> 8) & 0xFF;
    buffer[7] = STUN_MAGIC_COOKIE & 0xFF;

    // Transaction ID: 12 bytes (random)
    for (int i = 8; i < 20; i++)
    {
        buffer[i] = (uint8_t)(rand() % 256);
    }
}

bool parseMappedAddress(const uint8_t* response, int responseLength, std::string& ip, int& port)
{
    if (responseLength < 20) return false;

    uint16_t msgType = (response[0] << 8) | response[1];
    if (msgType != 0x0101) return false;

    uint16_t msgLength = (response[2] << 8) | response[3];
    if (msgLength + 20 > responseLength) return false;

    // Attributes start at offset 20
    int offset = 20;
    while (offset + 4 <= responseLength)
    {
        uint16_t attrType = (response[offset] << 8) | response[offset + 1];
        uint16_t attrLength = (response[offset + 2] << 8) | response[offset + 3];

        if (offset + 4 + attrLength > responseLength) break;

        if (attrType == 0x0001 || attrType == 0x0020)
        {
            // Structure:
            // 0: Reserved (1 byte)
            // 1: Family (1 byte) 0x01 = IPv4
            // 2-3: Port (2 bytes)
            // 4-7: IPv4 address (4 bytes)
            if (attrLength >= 8)
            {
                uint8_t family = response[offset + 5];
                if (family != 0x01) return false;

                uint16_t port_raw = (response[offset + 6] << 8) | response[offset + 7];
                uint32_t ip_raw = (response[offset + 8] << 24) | (response[offset + 9] << 16) |
                    (response[offset + 10] << 8) | (response[offset + 11]);

                if (attrType == 0x0001)
                {
                    port = port_raw;
                    ip = std::to_string(response[offset + 8]) + "." +
                        std::to_string(response[offset + 9]) + "." +
                        std::to_string(response[offset + 10]) + "." +
                        std::to_string(response[offset + 11]);
                    return true;
                }
                else if (attrType == 0x0020)
                {
                    // XOR-MAPPED-ADDRESS: XOR port and IP with magic cookie
                    port = port_raw ^ (STUN_MAGIC_COOKIE >> 16);
                    uint32_t ip_xor = ip_raw ^ STUN_MAGIC_COOKIE;
                    ip = std::to_string((ip_xor >> 24) & 0xFF) + "." +
                        std::to_string((ip_xor >> 16) & 0xFF) + "." +
                        std::to_string((ip_xor >> 8) & 0xFF) + "." +
                        std::to_string(ip_xor & 0xFF);
                    return true;
                }
            }
        }

        // Move to next attribute (4 byte header + attrLength, padded to 4 bytes)
        int paddedLength = (attrLength + 3) & ~3;
        offset += 4 + paddedLength;
    }

    return false;
}

std::string interpretNatType(bool udpBlocked, bool portPreserved1, bool portPreserved2, bool symmetricNat)
{
    if (udpBlocked)
        return "NAT Type: Strict";

    if (symmetricNat)
        return "Nat Type: Strict";

    if (portPreserved1 && portPreserved2)
    {
        // If ports preserved on both servers and equal, Full Cone NAT
        // Otherwise, Restricted NAT (ports preserved but differ)
        return (portPreserved1 == portPreserved2) ? "NAT Type: Open" : "NAT Type: Moderate";
    }
    else if (portPreserved1 || portPreserved2)
    {
        // Port preserved on one server only means restricted NAT
        return "NAT Type: Moderate";
    }

    return "NAT Type: Moderate";
}


bool sendStunRequest(SOCKET sock, const sockaddr* addr, int addrLen, const uint8_t* request, int reqLen, std::string& ipOut, int& portOut)
{
    if (sendto(sock, (const char*)request, reqLen, 0, addr, addrLen) == SOCKET_ERROR)
    {
        return false;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval tv;
    tv.tv_sec = 1; // 1 second timeout
    tv.tv_usec = 0;

    int res = select(0, &readfds, NULL, NULL, &tv);
    if (res > 0 && FD_ISSET(sock, &readfds))
    {
        uint8_t buffer[512];
        sockaddr_storage fromAddr;
        int fromLen = sizeof(fromAddr);
        int bytesReceived = recvfrom(sock, (char*)buffer, sizeof(buffer), 0, (sockaddr*)&fromAddr, &fromLen);
        if (bytesReceived > 0)
        {
            if (parseMappedAddress(buffer, bytesReceived, ipOut, portOut))
                return true;
        }
    }

    return false;
}

std::string detectNatType(int localPort, std::string& publicIpOut, int& publicPortOut)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return "WSAStartup failed";
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        WSACleanup();
        return "Failed to create socket";
    }

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(localPort);

    if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        WSACleanup();
        return "Failed to bind socket";
    }

    std::string localIp = getLocalIp(sock);

    addrinfo hints{}, * res1 = nullptr, * res2 = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(STUN_SERVER_1, STUN_PORT, &hints, &res1) != 0 ||
        getaddrinfo(STUN_SERVER_2, STUN_PORT, &hints, &res2) != 0)
    {
        closesocket(sock);
        WSACleanup();
        return "Failed to resolve STUN server addresses";
    }

    uint8_t stunReq[20];
    createStunRequest(stunReq);

    std::string ip1, ip2;
    int port1 = 0, port2 = 0;

    bool success1 = sendStunRequest(sock, res1->ai_addr, (int)res1->ai_addrlen, stunReq, sizeof(stunReq), ip1, port1);
    bool success2 = sendStunRequest(sock, res2->ai_addr, (int)res2->ai_addrlen, stunReq, sizeof(stunReq), ip2, port2);

    freeaddrinfo(res1);
    freeaddrinfo(res2);

    if (!success1 && !success2)
    {
        closesocket(sock);
        WSACleanup();
        return interpretNatType(true, false, false, false);
    }

    std::string ipUsed = success1 ? ip1 : ip2;
    int portUsed = success1 ? port1 : port2;

    publicIpOut = ipUsed;
    publicPortOut = portUsed;

    // Open Internet detection: local IP matches public IP
    if (localIp == publicIpOut)
    {
        closesocket(sock);
        WSACleanup();
        return "NAT Type: Open";
    }

    bool portPreserved1 = success1 && (port1 == localPort);
    bool portPreserved2 = success2 && (port2 == localPort);

    bool symmetricNat = false;
    if (success1 && success2)
    {
        symmetricNat = (ip1 != ip2) || (port1 != port2);
    }

    closesocket(sock);
    WSACleanup();

    return interpretNatType(false, portPreserved1, portPreserved2, symmetricNat);
}
