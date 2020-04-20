#ifndef SIDECAR_ALGORITHMS_PASTBUFFER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_PASTBUFFER_H

#include <algorithm>
#include <deque>
#include <functional>

namespace ZCHX {
namespace Algorithms {

class PastBufferBase {
public:
    PastBufferBase(size_t capacity) : capacity_(capacity), maxMsgSize_(0) {}

    size_t getCapacity() const { return capacity_; }

    size_t getMaxMsgSize() const { return maxMsgSize_; }

    void setCapacity(size_t capacity) { capacity_ = capacity; }

protected:
    void setMaxMsgSize(size_t size) { maxMsgSize_ = size; }

private:
    size_t capacity_;
    size_t maxMsgSize_;
};

template <typename T>
class PastBuffer : public PastBufferBase {
public:
    using MsgDeque = std::deque<typename T::Ref>;
    using const_iterator = typename MsgDeque::const_iterator;

    PastBuffer(size_t capacity) : PastBufferBase(capacity), msgs_() {}

    size_t size() const { return msgs_.size(); }

    bool empty() const { return msgs_.empty(); }

    bool full() const { return msgs_.size() == getCapacity(); }

    void clear()
    {
        msgs_.clear();
        setMaxMsgSize(0);
    }

    void add(typename T::Ref msg)
    {
        msgs_.push_front(msg);
        while (msgs_.size() > getCapacity()) { msgs_.pop_back(); }

        if (msg->size() < getMaxMsgSize()) {
            msg->resize(getMaxMsgSize());
        } else if (msg->size() > getMaxMsgSize()) {
            setMaxMsgSize(msg->size());
            std::for_each(msgs_.begin() + 1, msgs_.end(), ResizeMsg(getMaxMsgSize()));
        }
    }

    typename T::Ref operator[](int index) const { return msgs_[index]; }

    typename T::Ref back() const { return msgs_.back(); }

    typename T::Ref front() const { return msgs_.front(); }

    void pop_back() { msgs_.pop_back(); }

    void pop_front() { msgs_.pop_front(); }

    const_iterator begin() const { return msgs_.begin(); }

    const_iterator end() const { return msgs_.end(); }

private:
    struct ResizeMsg {
        size_t size_;

        ResizeMsg(size_t size) : size_(size) {}

        void operator()(typename T::Ref msg) const { msg->resize(size_, 0); }
    };

    MsgDeque msgs_;
};

} // end namespace Algorithms
} // end namespace SideCar

#endif
