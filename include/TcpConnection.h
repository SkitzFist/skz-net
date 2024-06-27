#include <WS2tcpip.h>
#include <WinSock2.h>
#include <memory>
#include <mutex>
#include <string>

#include "MessageHeader.h"
#include "SharedBuffer.h"

// debug
#include <iostream>

// todo should move to client/server interface
void windowsStartup() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(-1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        fprintf(stderr, "Version 2.2 of Winsock is not available.\n");
        WSACleanup();
        exit(-2);
    }
}

namespace SkzNet {

constexpr const int MAX_DATA_SIZE = 1024;
struct TcpConnection {
  public:
    TcpConnection(bool ipv4,
                  const std::string &ip,
                  const std::string &port,
                  bool shouldBind,
                  std::shared_ptr<SharedBuffer> sharedInBuffer) : IPV4(ipv4),
                                                                  m_sharedInBuffer(sharedInBuffer) {

        windowsStartup(); // todo should move to client/server interface

        m_socket = initSocket(IPV4, ip, port, shouldBind);

        if (m_socket != -1) {
            m_isConnected = true;
        } else {
            m_isConnected = false;
        }
    }

    TcpConnection(int socket,
                  std::shared_ptr<SharedBuffer> sharedInBuffer) : m_socket(socket),
                                                                  m_isConnected(true),
                                                                  m_sharedInBuffer(sharedInBuffer) {
    }

    ~TcpConnection() {
        std::scoped_lock lock(m_mutex);
        if (m_socket != -1) {
            close();
        }

        // todo maybe do startup/cleanup somewhere else
        WSACleanup();
    }

    bool isConnected() {
        return m_isConnected;
    } // todo should check connection as well

    int getSocket() {
        return m_socket;
    }

    void sendMessage(const void *message, const size_t size) {
        if (size <= 0) {
            return;
        }

        memcpy(m_outBuffer, message, size);

        size_t totalBytes = size;
        size_t sentBytes = 0;
        size_t missingBytes = totalBytes;

        while (sentBytes != totalBytes) {
            int bytes = send(m_socket, m_outBuffer + sentBytes, missingBytes, 0);
            if (bytes == -1) {
                disconnect();
                return;
            }

            sentBytes += bytes;
            missingBytes = totalBytes - sentBytes;
        }
    }

    void sendMessage(char *data, size_t size) {
        if (size <= 0) {
            return;
        }

        size_t totalBytes = size;
        size_t sentBytes = 0;
        size_t missingBytes = totalBytes;

        while (sentBytes != totalBytes) {
            int bytes = send(m_socket, data + sentBytes, missingBytes, 0);
            if (bytes == -1) {
                disconnect();
                return;
            }

            sentBytes += bytes;
            missingBytes = totalBytes - sentBytes;
        }
    }

    void receiveMessage() {
        int numbytes = recv(m_socket, m_inBuffer, MAX_DATA_SIZE - 1, 0);

        if (numbytes == -1) {
            disconnect();
            return;
        } else if (numbytes == 0) {
            std::cerr << "connection was closed" << '\n';
            disconnect();
            return;
        }

        if (!m_sharedInBuffer->write(m_inBuffer, (size_t)numbytes)) {
            std::cout << "Could not write to shared buffer" << '\n';
        }

        m_hasPendingMessages = true;
    }

    void close() {
        closesocket(m_socket);
        m_socket = -1;
        std::cout << "Socket Closed" << '\n';
    }

    void disconnect() {
        std::scoped_lock lock(m_mutex);
        close();
        m_isConnected = false;
    }

    bool hasPendingMessages() {
        return m_hasPendingMessages;
    }

  private:
    std::mutex m_mutex;
    int m_socket;
    char m_inBuffer[MAX_DATA_SIZE];
    char m_outBuffer[MAX_DATA_SIZE];
    bool IPV4;
    bool m_isConnected;
    bool m_hasPendingMessages;

    // todo should be sharedPtr
    std::shared_ptr<SharedBuffer> m_sharedInBuffer;

    int initSocket(bool ipv4, const std::string &ip, const std::string &port, bool shouldBind) {
        struct addrinfo hints, *servinfo, *p;
        int status;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = ipv4 ? AF_INET : AF_INET6;
        hints.ai_socktype = SOCK_STREAM;

        if (ip.empty() && shouldBind) {
            hints.ai_flags = AI_PASSIVE;

            if ((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
                return -1;
            }
        } else {
            if ((status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
                return -1;
            }
        }

        int socketFd = -1;
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                continue;
            }

            if (shouldBind) {
                if (bind(socketFd, p->ai_addr, p->ai_addrlen) == -1) {
                    closesocket(socketFd);
                    continue;
                }
            } else {
                if (connect(socketFd, p->ai_addr, p->ai_addrlen) == -1) {
                    closesocket(socketFd);
                    continue;
                }
            }
        }

        freeaddrinfo(servinfo);

        return socketFd;
    }
};

} // namespace SkzNet