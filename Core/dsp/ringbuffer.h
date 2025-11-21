#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

#include "../config.h"

namespace {
    const int default_count = 64;
    const int spin_count = 100;
    #define ALIGN (8)
};

template<typename T, size_t max_count = default_count> class ringbuffer {
    typedef T* TPtr;

public:
    ringbuffer():
        read_index(0),
        write_index(0),
        blocks_available(0),
        emptyCount(0),
        fullCount(0),
        writeCount(0),
        stopped(false)
    {
    }

    ~ringbuffer()
    {
        TracePrintln("ringbuffer", "");

        Stop();
    }

    int getFullCount() const { return fullCount; }

    int getEmptyCount() const { return emptyCount; }

    int getWriteCount() const { return writeCount; }

    void Start()
    {
        std::unique_lock<std::mutex> lk(mutex);
        write_index = read_index = 0;
        stopped = false;
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lk(mutex);
        read_index = 0;
        stopped = true;
        write_index = max_count / 2;
        nonfullCV.notify_all();
        nonemptyCV.notify_all();
    }

    void setBlockSize(int size)
    {
        TracePrintln("ringbuffer", "");

        if (block_size != size)
        {
            block_size = size;

            int aligned_block_size = (block_size + ALIGN - 1) & (~(ALIGN - 1));

            DebugPrintln("ringbuffer", "New raw buffer size : %ld", max_count * aligned_block_size);

            for(auto it = buffers.begin(); it < buffers.end(); it++)
            {
                it->resize(aligned_block_size);
            }
        }
    }

    T* peekWritePtr(int offset)
    {
        return buffers[(write_index.load() + max_count + offset) % max_count].data();
    }

    T* peekReadPtr(int offset)
    {
        return buffers[(read_index.load() + max_count + offset) % max_count].data();
    }

    void push(vector<T> arr)
    {
        WaitUntilNotFull();

        std::unique_lock<std::mutex> lk(mutex);

        buffers[write_index] = arr;

        write_index = (write_index + 1) % max_count;
        blocks_available++;

        if (blocks_available == 1)
        {
            nonemptyCV.notify_all();
        }

        writeCount++;
    }

    vector<T> pop()
    {
        WaitUntilNotEmpty();

        std::unique_lock<std::mutex> lk(mutex);

        vector<T> vec = buffers[read_index];

        read_index = (read_index + 1) % max_count;
        blocks_available--;

        if (blocks_available == max_count - 1)
        {
            nonfullCV.notify_all();
        }

        return vec;
    }

    int getBlockSize() const { return block_size; }

    void WaitUntilNotEmpty()
    {
        if (stopped) return;

        // if not empty
        for (int i = 0; i < spin_count; i++)
        {
            if (blocks_available > 0)
                return;
        }

        //if(log2) printf("read buffer empty %ld\n", blocks_available.load());

        if(blocks_available <= 0)
        {
            std::unique_lock<std::mutex> lk(mutex);

            emptyCount++;
            nonemptyCV.wait(lk, [this] {
                //if(log2) {printf("read buffer should be non empty : %d\n", (blocks_available.load() > 0));}
                return blocks_available > 0;
            });
        }
    }

    void WaitUntilNotFull(bool log = false)
    {
        if (stopped) return;

        for (int i = 0; i < spin_count; i++)
        {
            if (blocks_available < max_count)
                return;
        }

        //if(log) printf("read buffer full %ld, %ld\n", blocks_available.load(), max_count);

        if (blocks_available >= max_count)
        {
            std::unique_lock<std::mutex> lk(mutex);
            fullCount++;
            nonfullCV.wait(lk, [this] {
                return blocks_available < max_count;
            });
        }
    }

    volatile atomic<size_t> read_index;
    volatile atomic<size_t> write_index;
    volatile atomic<size_t> blocks_available;

private:
    int emptyCount;
    int fullCount;
    int writeCount;

    std::mutex mutex;
    bool stopped;
    std::condition_variable nonemptyCV;
    std::condition_variable nonfullCV;

    int block_size = 0;

    array<vector<T>, max_count> buffers;
};