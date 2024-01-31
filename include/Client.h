#ifndef SKZ_NET_CLIENT_H_
#define SKZ_NET_CLIENT_H_

#include <thread>
#include <memory>
#include <string>
#include <stdint.h>
#include <iostream>

#include "asio.hpp"
#include "ThreadSafeQueue.h"
#include "Message.h"
#include "Connection.h"

namespace SkzNet{

  template <typename T>
  class ClientInterface{
    public:
      ClientInterface() : m_sokcet(m_context){

      }


      virtual ~ClientInterface(){
        disconnect();
      }

    public:
      bool connect(const std::string& host, const uint16_t port){
        try{
          m_connection = std::make_unique<Connection<T>>();
          asio::ip::tcp::resolver resolver(m_context);
          m_endpoints = resolver.resolve(host, std::to_string(port));

          m_connection->connectToServer(m_endpoints);
          threadContext = std::thread([this]() { m_context.run(); } );

        }catch(std::exception& e){
          //todo client/server should provide it's own logging tool.
          std::cerr << "Client could not connect: " << e.what << '\n';
        }

        return false;
      }

      void disconnect(){
        if(isConnected()){
          m_connection->disconnect();
        }

        m_context.stop();
        if(threadContext.joinable()){
          threadContext.join();
        }

        m_connection.release();
      }

      bool isConnected(){
        if(m_connection){
          return m_connection->isConnected();
        }else{
          return false;
        }
      }

      ThreadSafeQueue<OwnedMessage<T>>& incoming(){
        return m_messagesIn;
      }

    protected:
    asio::io_context m_context;
    std::thread threadContext;
    asio::ip::tcp::socket m_socket;
    std::unique_ptr<Connection<T>> m_connection;

    private:
    ThreadSafeQueue<OwnedMessage<T>> m_messagesIn;

  };

};

#endif