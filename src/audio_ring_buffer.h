#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace picointerface {

struct AudioFrame {
    std::int32_t left;
    std::int32_t right;
};

class AudioRingBuffer {
public:
    static constexpr std::size_t capacity = 256;

    bool push(AudioFrame frame) noexcept {
        const std::size_t next_head = (head_ + 1) & (capacity - 1);
        if (next_head == tail_) {
            ++dropped_frames_;
            return false;
        }

        frames_[head_] = frame;
        head_ = next_head;
        return true;
    }

    bool pop(AudioFrame& frame) noexcept {
        if (tail_ == head_) {
            return false;
        }

        frame = frames_[tail_];
        tail_ = (tail_ + 1) & (capacity - 1);
        return true;
    }

    void clear() noexcept {
        tail_ = head_;
    }

    std::uint32_t dropped_frames() const noexcept {
        return dropped_frames_;
    }

private:
    static_assert((capacity & (capacity - 1)) == 0);

    std::array<AudioFrame, capacity> frames_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::uint32_t dropped_frames_ = 0;
};

}