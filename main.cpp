#include <algorithm>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

// вспомогательная функция для invert_bits
// работает на блоке [begin, end) последовательно
template <typename Function>
void invert_bits_sequential(void* mem, std::size_t begin, std::size_t end, Function func)
{
    auto bytes = static_cast<uint8_t*>(mem);
    for (auto i = begin; i != end; ++i)
        for (std::uint8_t j = 0; j < 8; ++j) {
            const bool bit = func(8 * i + j);
            bytes[i] ^= bit << (7-j);
        }
}

// главная функция, в основном состоит из управления потоками
// сама работа выполняеться в invert_bits_sequential на разных потоках
// Function func определяет, нужно ли инвертировать i-й бит
// Function func должна принимать одну переменную, которую можно конвертировать в std::size_t
// Function func должна возвращать тип, который может конвертироваться в bool
template <typename Function>
void invert_bits(void* mem, std::size_t size, Function func)
{
    constexpr std::size_t min_job = 1000;
    const std::size_t max_threads = std::max(1u, std::thread::hardware_concurrency());
    const std::size_t num_threads = std::min(max_threads, (size + min_job - 1) / min_job);
    const std::size_t block_size = size / num_threads;
    
    std::vector<std::thread> threads;
    std::size_t begin = 0;
    for (std::size_t i = 0; i < num_threads - 1; ++i) {
        const auto end = begin + block_size;
        threads.emplace_back(invert_bits_sequential<Function>, mem, begin, end, func);
        begin = end;
    }
    invert_bits_sequential(mem, begin, size, func);
    
    for (auto& thread : threads) {
        thread.join();
    }
}

constexpr int array_size = 100'000'000;
std::uint8_t array[array_size]{};

// печатает первые 4 байта массива для дебага
void print(std::uint8_t *a)
{
    for (int i = 0; i < std::min(4, array_size); ++i) {
        std::cout << std::bitset<8>{a[i]};
    }
    std::cout << "\n\n";
}

// здесь мы сравниваем последовательную и параллальную версии invert_bits на скорость
int main()
{
    print(array);
    
    auto start = std::chrono::high_resolution_clock::now();
    invert_bits_sequential(static_cast<void*>(array), 0, sizeof(array), [](std::size_t i){ return i % 9 == 0; });
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Sequential " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
    
    print(array);
    
    start = std::chrono::high_resolution_clock::now();
    invert_bits(static_cast<void*>(array), sizeof(array), [](std::size_t i){ return i % 9 == 0; });
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Parallel " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
    
    print(array);
    
    return 0;
}