#include <SDL3/SDL.h>
#include <atomic>
#include <cstdint>
#include <vector>

// only really for audio use, made in a separate class for organisation
// works as a FIFO queue
class RingBuffer {
public:
    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool full() const;

    // capacity is the max data size allowed in the buffer
    [[nodiscard]] size_t capacity() const;
    // size is the amount of data currently in the buffer
    [[nodiscard]] size_t size() const;

    void setSize(size_t audioBufferSize, size_t numAudioBuffers);

    // doesn't return any data as internal handling is easier
    // push returns data read from stream, pop returns data written to stream
    // returns -1 for invalid state
    int32_t push(SDL_AudioStream* stream, int available);
    int32_t pop(SDL_AudioStream* stream, int needed);

private:
    // data size is audiobuffersize * maxbuffers
    std::vector<uint8_t> m_data;
    // size is audiobuffersize
    std::vector<uint8_t> m_emptyBuffer;

    // head is first in
    std::atomic<size_t> m_head;
    // tail is last in
    std::atomic<size_t> m_tail;
    std::atomic<bool> m_full = true;

    size_t m_audioBufferSize;
    size_t m_numAudioBuffers;
};