/*
 * This file is part of SDDC_Driver.
 *
 * =====================
 *      MIT License
 * =====================
 *
 * Copyright (C) 2021 - Howard Su
 * Copyright (C) 2025 - RenardSpark
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <array>
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

    void push(std::vector<T> arr)
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

    std::vector<T> pop()
    {
        WaitUntilNotEmpty();

        std::unique_lock<std::mutex> lk(mutex);

        std::vector<T> vec = buffers[read_index];

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

        if(blocks_available <= 0)
        {
            std::unique_lock<std::mutex> lk(mutex);

            emptyCount++;
            nonemptyCV.wait(lk, [this] {
                return blocks_available > 0;
            });
        }
    }

    void WaitUntilNotFull()
    {
        if (stopped) return;

        for (int i = 0; i < spin_count; i++)
        {
            if (blocks_available < max_count)
                return;
        }

        if (blocks_available >= max_count)
        {
            std::unique_lock<std::mutex> lk(mutex);
            fullCount++;
            nonfullCV.wait(lk, [this] {
                return blocks_available < max_count;
            });
        }
    }

    volatile std::atomic<size_t> read_index;
    volatile std::atomic<size_t> write_index;
    volatile std::atomic<size_t> blocks_available;

private:
    int emptyCount;
    int fullCount;
    int writeCount;

    std::mutex mutex;
    bool stopped;
    std::condition_variable nonemptyCV;
    std::condition_variable nonfullCV;

    int block_size = 0;

    std::array<std::vector<T>, max_count> buffers;
};