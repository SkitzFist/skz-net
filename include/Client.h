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
      ClientInterface(){

      }


      virtual ~ClientInterface(){
        disconnect(); 
      }

    public:
      void connect(const std::string& host, const uint16_t port){
        try{
          asio::ip::tcp::resolver resolver(m_context);
          asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

          m_connection = std::make_unique<Connection<T>>(
            Connection<T>::Owner::CLIENT,
            m_context,
            asio::ip::tcp::socket(m_context),
            m_messagesIn
          );

          m_connection->connectToServer(endpoints);

          threadContext = std::thread([this]() { m_context.run(); } );
        }catch(std::exception& e){
          //todo client/server should provide it's own logging tool.
          std::cerr << "Client could not connect: " << e.what() << '\n';
        }
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

      void send(const Message<T>& msg){
        if(isConnected()){
          m_connection->send(msg);
        }
      }

    protected:
    asio::io_context m_context;
    std::thread threadContext;
    std::unique_ptr<Connection<T>> m_connection;

    private:
    ThreadSafeQueue<OwnedMessage<T>> m_messagesIn;

  };

};

#endif