#ifndef SKZ_NET_CONNECTION_H_
#define SKZ_NET_CONNECTION_H_

#include <stdint.h>
#include <memory>

#include "asio.hpp"

#include "ThreadSafeQueue.h"
#include "message.h"

namespace SkzNet{

  template <typename T>
  class Connection : public std::enable_shared_from_this<Connection<T>>
  {
  public:
    enum class Owner{
      SERVER,
      CLIENT
    };

    Connection(
      Owner parent, 
      asio::io_context& asioContext, 
      asio::ip::tcp::socket socket, 
      ThreadSafeQueue<OwnedMessage<T>>& messagesIn)
      : m_asioContext(asioContext), 
        m_socket(std::move(socket)), 
        m_messagesIn(messagesIn){
          m_OwnerType = parent;
    }

    virtual ~Connection()
    {
    }

  public:
    bool connectToServer(){

    }

    bool connectToClient(uint32_t id = 0){
      if(m_OwnerType == Owner::SERVER){
        m_id = id;
        readHeader();
        return true;
      }

      return false;
    }
    
    bool disconnect(){
      if(isConnected()){
        asio::post(
          m_asioContext,
          [this](){
            m_socket.close();
            std::cout << "[" << m_id << "] DISCONNECTED" << '\n';
          }
        );
      }
    }
    
    bool isConnected() const{
      return m_socket.is_open();
    }
    
    bool send(const Message<T> &msg){
      asio::post(
        m_asioContext,
        [this, msg](){
          //assume that if there exists messages, a writing is already in progress
          //the writing will then handle this message as well.
          //trying to write if already writing can create race conditions.
          bool isWritingMessage = !m_messagesOut.isEmpty();
          m_messagesOut.push_back(msg);
          if(!isWritingMessage){
            writeHeader();
          }
        }
      );
    }

    uint32_t getId() const{
      return m_id;
    }    

  protected:
    asio::io_context &m_asioContext;
    asio::ip::tcp::socket m_socket;
    ThreadSafeQueue<Message<T>> m_messagesOut;
    ThreadSafeQueue<OwnedMessage<T>>& m_messagesIn;
    Message<T> m_msgTemporaryIn;

    Owner m_OwnerType = Owner::SERVER;
    uint32_t m_id = 0;

  private:
    void readHeader(){
      asio::async_read(
        m_socket,
        asio::buffer(&m_msgTemporaryIn.header, sizeof(MessageHeader<T>)),
        [this](std::error_code ec, std::size_t length){
          if(ec){
            std::cout << "[" << m_id << "] Read header failed" << '\n';
            m_socket.close();
          }else{
            if(m_msgTemporaryIn.header.size > 0){
              m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
              readBody();
            }else{
              addToIncomingMessageQueue();
            }
          }
        }
      );
    }

    void readBody(){
      asio::async_read(
        m_socket, 
        asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
        [this](std::error_code ec, std::size_t length){
          if(ec){
            std::cout << "[" << m_id << "] Read body failed" << '\n';
            m_socket.close();
          }else{
            addToIncomingMessageQueue();
          }
        }
      );
    }

    void addToIncomingMessageQueue(){
      if(m_OwnerType == Owner::SERVER){
        m_messagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
      }else{
        m_messagesIn.push_back({ nullptr, m_msgTemporaryIn });
      }

      readHeader();
    }

    void writeHeader(){
      asio::async_write(
        m_socket,
        asio::buffer( &m_messagesOut.front().header , sizeof( MessageHeader<T> ) ),
        [this](std::error_code ec, std::size_t length){
          if(ec){
            std::cout << "[" << m_id << "] Write header failed" << '\n';
            m_socket.close();
          } else {
            if( m_messagesOut.front().body.size > 0 ){
              writeBody();
            } else {
              m_messagesOut.popFront();

              if( !m_messagesOut.isEmpty() ){
                writeHeader();
              }
            }
          }
        }
      );
    }

    void writeBody(){
      asio::async_write(
        m_socket,
        asio::buffer(m_messagesOut.front().body.data(), m_messagesOut.front().body.size()),
        [this](std::error_code ec, std::size_t length){
          if(ec){
            std::cout << "[" << m_id << "] Write body failed" << '\n';
            m_socket.close();
          }else{
            m_messagesOut.popFront();
            if(!m_messagesOut.isEmpty()){
              writeHeader();
            }
          }
        }
      );
    }
  };

}; //SkzNet

#endif