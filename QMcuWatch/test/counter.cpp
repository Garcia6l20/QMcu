#include <chrono>
#include <random>
#include <thread>

#include <print>

int counter      = 0;
int noiseCounter = 0;

static constexpr auto n_data = 16;

int buffersIndex           = 0;
int counterBuffer[n_data]     = {42};
int counterBuffers[2][n_data] = {{0}};

int main()
{
  std::random_device                 rd;
  std::uniform_int_distribution<int> dist(-10, 10);
  while(true)
  {
    ++counter;
    noiseCounter = counter + dist(rd);

    std::println("counter: {}", counter);

    counterBuffer[buffersIndex]     = counter;
    counterBuffers[buffersIndex][0] = counter;
    counterBuffers[buffersIndex][1] = noiseCounter;

    ++buffersIndex;
    if(buffersIndex >= n_data)
    {
      buffersIndex = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  return 0;
}