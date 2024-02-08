#ifndef SKZ_NET_MESSAGE_H_
#define SKZ_NET_MESSAGE_H_

#include <stdint.h>
#include <vector>
#include <ostream>
#include <cstring>
#include <memory>

namespace SkzNet
{
  template <typename T>
  struct MessageHeader{
    T id{};
    uint32_t size = 0;
  };

  template <typename T>
  struct Message{
    MessageHeader<T> header{};
    std::vector<uint8_t> body;

    std::size_t size() const{
      return body.size();
    }

    //print
    friend std::ostream& operator << (std::ostream& os, const Message<T>& msg){
      os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
      return os;
    }

    //write to data buffer
    template <typename DataType>
    friend Message<T>& operator << (Message& msg, const DataType& data){
      static_assert(std::is_standard_layout<DataType>::value,"Data is to complex to be pushed on buffer");
      
      size_t i = msg.body.size();
      /*
        Todo resizing like this is bad practice!!!
        the user could add data in a loop, then it would resize the vector each iteration.
        should add a batch add method as well.
      */
      msg.body.resize(msg.body.size() + sizeof(DataType));
      std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
      msg.header.size = msg.size();

      return msg;
    }

    //get data
    template <typename DataType>
    friend Message<T>& operator >> (Message<T>& msg, DataType& data){
      static_assert(std::is_standard_layout<DataType>::value, "Data is to complex to be pushed on buffer");

      size_t i = msg.body.size() - sizeof(DataType);
      std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
      msg.body.resize(i);
      msg.header.size = msg.size();

      return msg;
    }

  };

  template <typename T>
  class Connection;

  template <typename T>
  struct OwnedMessage{
    std::shared_ptr<Connection<T>> remote = nullptr;
    Message<T> msg;

    friend std::ostream& operator << (std::ostream& os, const OwnedMessage<T>& msg){
      os << msg.msg;
      return os;
    }
    
  };

};

#endif