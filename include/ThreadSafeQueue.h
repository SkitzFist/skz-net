#ifndef SKZ_NET_THREAD_SAFE_QUEUE_H_
#define SKZ_NET_THREAD_SAFE_QUEUE_H_

#include <mutex>
#include <deque>

namespace SkzNet{

  template<typename T>
  class ThreadSafeQueue{
    public:
      ThreadSafeQueue() = default;
      ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
      virtual ~ThreadSafeQueue() { clear(); }

      const T& front(){
        std::scoped_lock lock(muxQueue);
        return deqQueue.front();
      }

      T popFront(){
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.front());
        deqQueue.pop_front();
        return t;
      }

      const T& back(){
        std::scoped_lock lock(muxQueue);
        return deqQueue.back();
      }

      T popBack(){
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.back());
        deqQueue.pop_back();
        return t;
      }

      void push_back(const T& item){
        std::scoped_lock lock(muxQueue);
        deqQueue.emplace_back(std::move(item));
      }

      void push_front(const T& item){
        std::scoped_lock lock(muxQueue);
        deqQueue.emplace_front(std::move(item));
      }

      bool isEmpty(){
        std::scoped_lock lock(muxQueue);
        return deqQueue.empty();
      }

      size_t count(){
        std::scoped_lock lock(muxQueue);
        return deqQueue.size();
      }

      void clear(){
        std::scoped_lock lock(muxQueue);
        deqQueue.clear();
      }

    protected:
      std::mutex muxQueue;
      std::deque<T> deqQueue;
  };

};

#endif