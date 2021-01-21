#pragma once

#include <condition_variable>
#include <mutex>
#include <list>

namespace Thread
{
    template<typename T>
    class Channel
    {
    public:
        void Write(const T& data)
        {
            std::unique_lock<std::mutex> lock(mtx);
            buffer.push_back(data);
            cv.notify_all();
        }

        T Read()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() { return !buffer.empty(); });
            const auto item = buffer.front();
            buffer.pop_front();
            return item;
        }

        auto Length() const
        {
            return buffer.size();
        }
    	
    private:
        std::list<T> buffer{};
        std::mutex mtx{};
        std::condition_variable cv{};
    };
}
