#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_oldnames.h>
#include <cstddef>
#include <cstdint>
#include <ring_buffer.hpp>
#include <span>
#include <vector>

// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/ used as a guide for logic
bool RingBuffer::empty() const { return !m_full && m_head == m_tail; }
bool RingBuffer::full() const { return m_full; }

size_t RingBuffer::capacity() const { return m_data.size(); }
size_t RingBuffer::size() const {
    if(m_full) {
        return capacity();
    }

    if(m_head >= m_tail) {
        return m_head - m_tail;
    }

    return capacity() + m_head - m_tail;
}

void RingBuffer::setSize(size_t audioBufferSize, size_t numAudioBuffers) {
    m_audioBufferSize = audioBufferSize;
    m_numAudioBuffers = numAudioBuffers;

    m_data.clear();
    m_data.resize(audioBufferSize * numAudioBuffers);

    m_emptyBuffer = std::vector<uint8_t>(audioBufferSize, 0);

    m_tail = 0;
    m_head = 0;
    m_full = false;
}

// this isnt the most safe implementation, more validation should be done with setting head to secondSize, etc but i dont think any crashes should happen
// there may be async issues, but again, no crashes should happen and it would be pretty rare so for speed dont care unless i find crashes happen
int32_t RingBuffer::push(SDL_AudioStream* stream, int available) {
    if(m_data.empty()) {
        return -1;
    }

    while(available >= static_cast<int>(m_audioBufferSize)) {
        if(m_tail + m_audioBufferSize >= m_data.size()) {
            size_t firstSize  = m_data.size() - m_tail;
            size_t secondSize = m_audioBufferSize - firstSize;

            std::span<uint8_t> firstHalf(m_data.begin() + static_cast<long>(m_tail), m_data.end());
            std::span<uint8_t> secondHalf(m_data.begin(), m_data.begin() + static_cast<long>(secondSize));

            int numRead  = SDL_GetAudioStreamData(stream, firstHalf.data(), static_cast<int32_t>(firstHalf.size_bytes()));
            int numRead2 = 0;
            if(numRead == -1) {
                return -1;
            }

            if(secondSize != 0) {
                numRead2 = SDL_GetAudioStreamData(stream, secondHalf.data(), static_cast<int32_t>(secondHalf.size_bytes()));
                if(numRead2 == -1) {
                    return -1;
                }
            }

            if(m_head >= m_tail || m_head < secondSize) {
                m_head = secondSize;
                m_full = true;
            }

            available -= numRead + numRead2;
            m_tail = secondSize;

            if(m_head == m_tail) {
                m_full = true;
            }

            continue;
        }

        auto beginIt = m_data.begin() + static_cast<long>(m_tail);
        std::span<uint8_t> bufferSpan(beginIt, beginIt + static_cast<long>(m_audioBufferSize));

        int numRead = SDL_GetAudioStreamData(stream, bufferSpan.data(), static_cast<int32_t>(bufferSpan.size_bytes()));
        if(numRead == -1) {
            return -1;
        }

        available -= numRead;

        size_t prevTail = m_tail;
        m_tail += static_cast<size_t>(numRead);

        if(m_head >= prevTail && m_head < m_tail) {
            m_head.store(m_tail);
            m_full = true;
        }
        else if(m_head == m_tail) {
            m_full = true;
        }
    }

    return 0;
}

int32_t RingBuffer::pop(SDL_AudioStream* stream, int needed) {
    if(m_data.empty()) {
        return -1;
    }

    int32_t written = 0;
    while(needed >= static_cast<int>(m_audioBufferSize)) {
        if(size() < m_audioBufferSize) {
            SDL_PutAudioStreamData(stream, m_emptyBuffer.data(), static_cast<int32_t>(m_emptyBuffer.size()));

            written += static_cast<int32_t>(m_emptyBuffer.size());
            needed -= static_cast<int>(m_emptyBuffer.size());

            continue;
        }

        if(m_head + m_audioBufferSize >= capacity()) {
            size_t firstSize  = m_data.size() - m_head;
            size_t secondSize = m_audioBufferSize - firstSize;

            std::span<uint8_t> firstHalf(m_data.begin() + static_cast<long>(m_head), m_data.end());
            std::span<uint8_t> secondHalf(m_data.begin(), m_data.begin() + static_cast<long>(secondSize));

            if(
                !SDL_PutAudioStreamData(stream, firstHalf.data(), static_cast<int32_t>(firstHalf.size_bytes())) ||
                (secondSize != 0 && !SDL_PutAudioStreamData(stream, secondHalf.data(), static_cast<int32_t>(secondHalf.size_bytes())))
            ) {
                return -1;
            }

            needed -= static_cast<int>(m_audioBufferSize);
            written += static_cast<int32_t>(m_audioBufferSize);

            m_head = secondSize;
            m_full = false;

            continue;
        }

        auto beginIt = m_data.begin() + static_cast<long>(m_head);
        std::span<uint8_t> bufferSpan(beginIt, beginIt + static_cast<long>(m_audioBufferSize));

        if(!SDL_PutAudioStreamData(stream, bufferSpan.data(), static_cast<int32_t>(bufferSpan.size_bytes()))) {
            return -1;
        }

        needed -= static_cast<int>(m_audioBufferSize);
        written += static_cast<int32_t>(m_audioBufferSize);

        m_head += m_audioBufferSize;
        m_full = false;
    }

    return written;
}