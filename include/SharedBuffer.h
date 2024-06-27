#ifndef SKZ_NET_SHARED_BUFFER_H_
#define SKZ_NET_SHARED_BUFFER_H_

#include "MessageHeader.h"
#include <mutex>
#include <string>
#include <vector>

// debug
#include <iostream>
namespace SkzNet {

// todo should receive a max_size in ctr, as server needs more memory then client
constexpr const size_t MAX_SIZE = 2048;

// todo look into queue instead of linked list.
struct MemorySegment {
    size_t index;
    size_t size;
    bool isFree;
    MemorySegment *next;
    MemorySegment *prev;
};

struct MemorySegments {
    MemorySegment root;

    size_t reserve(const size_t &size) {
        // needs to be atomic or locked if space is found and when adding a new node.

        MemorySegment *ptr = &root;
        bool hasReserved = false;
        size_t index = -1;
        while (ptr && !hasReserved) {
            if (ptr->isFree && size <= ptr->size) {

                // create new free memory segment, todo should check newFreeSpace is 0
                size_t newFreeSpace = ptr->size - size;
                // todo should pool memorysegments
                MemorySegment *newSegment = new MemorySegment();
                newSegment->index = ptr->index + size;
                newSegment->size = newFreeSpace;
                newSegment->isFree = true;
                newSegment->next = ptr->next;
                newSegment->prev = ptr;

                // set this memoryFragment to new data
                index = ptr->index;
                ptr->isFree = false;
                ptr->size = size;
                ptr->next = newSegment;

                // check if nextNext is free, then absorb with next (Needs to be atomic/locked)
                if (newSegment->next && newSegment->next->isFree) {
                    MemorySegment *nextNext = newSegment->next->next;
                    newSegment->size += newSegment->next->size;
                    // todo should pool memorysegments
                    delete newSegment->next;
                    newSegment->next = nextNext;
                }
                hasReserved = true;
            }
            ptr = ptr->next;
        }

        if (!hasReserved) {
            std::cout << "[ERROR] Shared buffer -> could not allocate space" << '\n';
        }

        return index;
    }

    void merge(MemorySegment *a, MemorySegment *b) {
        a->size += b->size;
        a->next = b->next;
        if (a->next) {
            a->next->prev = a;
        }
        a->isFree = true;

        delete b;
    }

    void unreserve(const size_t &index) {
        MemorySegment *ptr = &root;
        bool hasUnreserved = false;

        while (ptr && !hasUnreserved) {

            if (ptr->index == index) {

                MemorySegment *prev = ptr->prev;
                MemorySegment *next = ptr->next;

                if (prev && prev->isFree) {
                    merge(prev, ptr);
                    ptr = prev;
                    if (ptr->next->isFree) {
                        merge(prev, prev->next);
                    }
                } else if (next && next->isFree) {
                    merge(ptr, next);
                } else {
                    ptr->isFree = true;
                }

                hasUnreserved = true;
            }

            ptr = ptr->next;
        }
    }
};

struct SharedBuffer {
  public:
    SharedBuffer() {
        m_segments.root.index = 0;
        m_segments.root.size = MAX_SIZE;
        m_segments.root.isFree = true;
        m_segments.root.next = nullptr;
        m_segments.root.prev = nullptr;
    }

    // todo change name
    size_t reserve(const size_t &size) {
        std::scoped_lock lock(mutex);
        return m_segments.reserve(size);
    }

    void unreserve(const size_t &index) {
        std::scoped_lock lock(mutex);
        m_segments.unreserve(index);
    }

    MessageHeader getMessageHeader(size_t index) {
        MessageHeader header;
        readHeader(m_data + index, header);
        return header;
    }

    size_t getMessageSize(size_t index) {
        size_t size;
        memcpy(&size, m_data + index, sizeof(size_t));
        return size;
    }

    char *getMessageData(size_t index) {
        return m_data + index + sizeof(MessageHeader);
    }

    void readMessage(void *message, const size_t &index) {
        size_t size = getMessageSize(index);
        memcpy(message, m_data + index, size);
    }

    bool write(const void *data, const size_t &size) {
        size_t index = reserve(size);
        if (index == (size_t)-1) {
            return false;
        }

        memcpy(m_data + index, data, size);
        return true;
    }

    bool hasPendingMessage() {
        MemorySegment *ptr = &m_segments.root;

        while (ptr) {
            if (!ptr->isFree) {
                return true;
            }
            ptr = ptr->next;
        }

        return false;
    }

    size_t getNextMessageIndex() {
        MemorySegment *ptr = &m_segments.root;

        while (ptr) {
            if (!ptr->isFree) {
                return ptr->index;
            }
            ptr = ptr->next;
        }

        return -1;
    }

    int getNumberOfPendingMessages() {
        std::scoped_lock lock(mutex);
        MemorySegment *ptr = &m_segments.root;

        int size = 0;

        while (ptr) {
            if (!ptr->isFree) {
                ++size;
            }
            ptr = ptr->next;
        }

        return size;
    }

    std::string visualize() {
        std::string str = "";

        MemorySegment *ptr = &m_segments.root;

        while (ptr) {
            std::string s = "<";
            s += ptr->isFree ? "+" : "-";
            s += std::to_string(ptr->size);
            s += " i:" + std::to_string(ptr->index) + "->";

            if (ptr->next) {
                s += "__";
            }
            str += s;
            ptr = ptr->next;
        }

        return str;
    }

    char *getData() {
        return m_data;
    }

  private:
    char m_data[MAX_SIZE];
    MemorySegments m_segments;
    std::mutex mutex;
};

} // namespace SkzNet
#endif
