#ifndef SKZ_NET_SERVER_H_
#define SKZ_NET_SERVER_H_

#include <stdint.h>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <deque>
#include <mswsock.h>

#include "asio.hpp"

#include "Connection.h"
#include "Message.h"
#include "ThreadSafeQueue.h"

namespace SkzNet{

  template <typename T>
  class serverInterface{
    public:
      serverInterface(uint16_t port)
        : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)){
          
      }

      virtual ~serverInterface(){}
    public:
      bool start(){
        try{
          waitForClientConnection();

          m_threadContext = std::thread([this](){
            m_asioContext.run();
          });



        }catch(std::exception& e){
          std::cerr << "[SERVER] EXCEPTION: " << e.what() << '\n';
          return false;
        }

        std::cout << "[SERVER] Started" << '\n';

        return true;
      }

      void stop(){
        m_asioContext.stop();

        if(m_threadContext.joinable()){
          m_threadContext.join();
        }

        std::cout << "[SERVER] Stopped" << '\n';
      }

      //async
      void waitForClientConnection(){
        m_asioAcceptor.async_accept(
          [this](std::error_code ec, asio::ip::tcp::socket socket){
             if(ec)             {
               std::cout << "[SERVER] New connection error: " << ec.message() << '\n';
             } else {
               std::cout << "[SERVER] New connection: " << socket.remote_endpoint() << '\n';

              std::shared_ptr<Connection<T>> newConnection = std::make_shared<Connection<T>>(
                Connection<T>::Owner::SERVER,
                m_asioContext,
                std::move(socket),
                m_messagesIn
              );
             
               if(onClientConnect(newConnection)){
                 m_deqConnections.push_back(std::move(newConnection));
                 m_deqConnections.back()->connectToClient(m_idCounter++);
                 std::cout << "[" << m_deqConnections.back()->getId() <<"] Connection approved" << '\n';

               }else{
                 std::cout << "[SERVER] Connection denied" << '\n';
               }
             }

             waitForClientConnection();
        });

      }

      void messageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg){
        if(client && client->isConnected()){
          client->send(msg);
        }else{
          //if client is no longer valid, disconnect it
          onClientConnect(client);
          client.reset();
          m_deqConnections.erase(
            std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
        }
      }

      void messageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignore = nullptr){
        bool foundInvalidClient = false;

        for(auto& client : m_deqConnections){
          if(client && client->isConnected()){
            if(client != ignore){
              client->send(msg);
            }
          }else{
            onClientDisconnect(client);
            client.reset();
            foundInvalidClient = true;
          }
        }

        if(foundInvalidClient){
          m_deqConnections.erase(
              std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
        }
      }

      void update(){
        if(m_messagesIn.count() > 0){
          std::cout << "[SERVER : UPDATE] Messages in queue: " << m_messagesIn.count() << '\n';
        }
        while (!m_messagesIn.isEmpty())
        {
          auto msg = m_messagesIn.popFront();
          onMessage(msg.remote, msg.msg);
        }
        
      }

    protected:
      virtual bool onClientConnect(std::shared_ptr<Connection<T>> client){
        (void)client;
        return false;
      }
      virtual void onClientDisconnect(std::shared_ptr<Connection<T>> client){
        (void)client;
      }
      virtual void onMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg){
        (void)client;
        (void)msg;
      }

    protected:
      ThreadSafeQueue<OwnedMessage<T>> m_messagesIn;

      std::deque<std::shared_ptr<Connection<T>>> m_deqConnections;

      asio::io_context m_asioContext;
      std::thread m_threadContext;
      asio::ip::tcp::acceptor m_asioAcceptor;

      uint32_t m_idCounter = 0;
  };

};

#endif