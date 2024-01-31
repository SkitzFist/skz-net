#ifndef SKZ_NET_CONNECTION_H_
#define SKZ_NET_CONNECTION_H_

#include "ThreadSafeQueue.h"
#include "message.h"
#include "asio.hpp"

namespace SkzNet{

  template <typename T>
  class Connection : public std::enable_shared_from_this<Connection<T>>
  {
  public:
    Connection()
    {
    }

    virtual ~Connection()
    {
    }

  public:
    bool connectToServer(){

    }
    
    bool disconnect(){

    }
    
    bool isConnected() const{

    }
    
    bool send(const Message<T> &msg){

    }
    

  protected:
    asio::ip::tcp::socket m_socket;
    asio::io_context &m_asioContext;
    ThreadSafeQueue<Message<T>> m_messagesOut;
    ThreadSafeQueue<Message<T>> m_messagesIn;
  };
};

#endif