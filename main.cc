#include <future>
#include <iostream>
#include <map>
#include <string.h>
#include <string>
#include <vector>

#include "chunk.hpp"
#include "data.hpp"
#include "mmap_file.hpp"
#include "shared_queue.hpp"
#include "timer.hpp"

using Result = std::map<std::string, Data>;

constexpr Chunk sentinel = {nullptr, 0};
constexpr uint32_t CHUNK_SIZE = 128 * 1024;
constexpr uint32_t MAX_LINE_LENGTH = 106;

void combineResult(Result &lhs, const Result &rhs) {
    for (const auto &[k, v] : rhs)
        lhs[k] = lhs.count(k) ? lhs[k].combine(rhs.at(k)) : rhs.at(k);
}

/*
 * \brief fast parse for 3..5-char strings with exactly one decimal
 * (range 99.9..99.9)
 */
float parseTemperature(const char *s) {
    const char *p = s;
    int sign = 1;
    if (*p == '-') {
        sign = -1;
        ++p;
    }
    /* pre = either 1-digit (p[1]=='.') or 2-digit; frac at p[2] or p[3] */
    int pre = (p[1] == '.') ? (p[0] - '0') : ((p[0] - '0') * 10 + (p[1] - '0'));
    int frac = p[(p[1] == '.') ? 2 : 3] - '0';
    int combined = pre * 10 + frac; /* integer representation scaled by 10 */
    return sign * (combined * 0.1f);
}

Result consumerThread(SharedQueue<Chunk> &queue) {
    Result res;
    while (true) {
        const Chunk chunk = queue.pop();
        if (chunk == sentinel)
            break;

        const char *itr =
            static_cast<const char *>(memchr(chunk.data, '\n', chunk.size));
        if (!itr)
            continue;
        ++itr;
        const char *end = chunk.data + chunk.size;
        std::string name;
        while (itr < end) {
            const char *sc_ptr =
                static_cast<const char *>(memchr(itr, ';', end - itr));
            if (!sc_ptr)
                break;
            name.assign(itr, sc_ptr);
            const float value = parseTemperature(sc_ptr + 1);
            auto &data = res[std::move(name)];
            ++data.occurences;
            data.sum += value;
            data.min = std::min(data.min, value);
            data.max = std::max(data.max, value);

            itr = static_cast<const char *>(
                memchr(sc_ptr + 1, '\n', end - (sc_ptr + 1)));
            if (!itr)
                break;
            ++itr;
        }

        if (itr) {
            const char *sc_ptr =
                static_cast<const char *>(memchr(itr, ';', MAX_LINE_LENGTH));
            if (sc_ptr) {
                name.assign(itr, sc_ptr);
                const float value = parseTemperature(sc_ptr + 1);
                auto &data = res[std::move(name)];
                ++data.occurences;
                data.sum += value;
                data.min = std::min(data.min, value);
                data.max = std::max(data.max, value);
            }
        }
    }
    return res;
}

int main(int argc, char **argv) {
    Timer timer;

    if (argc < 2 || argc > 3) {
        std::cerr << "Usage " << argv[0] << " <input_file> [n_workers]\n";
        return 1;
    }

    MMapFile file(argv[1]);
    const void *itr = file.data();
    const char *end = file.end();
    SharedQueue<Chunk> queue;

    // Consumers
    const uint32_t n_consumers =
        argc == 2 ? std::thread::hardware_concurrency() : std::stoul(argv[2]);
    std::vector<std::future<Result>> consumers;
    for (uint32_t i = 0; i < n_consumers; ++i) {
        consumers.push_back(
            std::async(std::launch::async, consumerThread, std::ref(queue)));
    }

    // Producer
    while (itr < end) {
        queue.push(
            {static_cast<const char *>(itr),
             std::min(CHUNK_SIZE, static_cast<uint32_t>(
                                      end - static_cast<const char *>(itr)))});
        itr = static_cast<const char *>(itr) + CHUNK_SIZE;
    }

    // Wait consumers and merge results
    Result result;
    for (uint32_t i = 0; i < n_consumers; ++i) {
        queue.push(sentinel);
    }
    for (auto &consumer : consumers) {
        Result res = consumer.get();
        combineResult(result, res);
    }

    // Final output
    for (const auto &[name, data] : result) {
        std::cout << name << ": " << data.min << "/"
                  << data.sum / data.occurences << "/" << data.max << "\n";
    }

    const double ms = timer.elapsedMs();
    std::cout << "Took: " << ms << "ms\n";

    return 1;
}
