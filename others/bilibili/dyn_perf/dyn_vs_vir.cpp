#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>

class Animal {
public:
    virtual ~Animal() = default;
    virtual int action() = 0;
};

class Dog : public Animal {
public:
    int action() override { return bark(); }
    int bark() { return 1; }
};

class Cat : public Animal {
public:
    int action() override { return meow(); }
    int meow() { return 2; }
};

class Bird : public Animal {
public:
    int action() override { return chirp(); }
    int chirp() { return 3; }
};

class Fish : public Animal {
public:
    int action() override { return swim(); }
    int swim() { return 4; }
};

class Rabbit : public Animal {
public:
    int action() override { return hop(); }
    int hop() { return 5; }
};


static void BM_VirtualFunction(benchmark::State& state) {
    std::vector<std::unique_ptr<Animal>> animals;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 4);
    
    for (int i = 0; i < 1000; ++i) {
        switch (dist(rng)) {
            case 0: animals.push_back(std::make_unique<Dog>()); break;
            case 1: animals.push_back(std::make_unique<Cat>()); break;
            case 2: animals.push_back(std::make_unique<Bird>()); break;
            case 3: animals.push_back(std::make_unique<Fish>()); break;
            case 4: animals.push_back(std::make_unique<Rabbit>()); break;
        }
    }
    
    int total = 0;
    for (auto _ : state) {
        for (auto& animal : animals) {
            total += animal->action();
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_VirtualFunction);

static void BM_DynamicCast(benchmark::State& state) {
    std::vector<std::unique_ptr<Animal>> animals;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 4);
    
    for (int i = 0; i < 1000; ++i) {
        switch (dist(rng)) {
            case 0: animals.push_back(std::make_unique<Dog>()); break;
            case 1: animals.push_back(std::make_unique<Cat>()); break;
            case 2: animals.push_back(std::make_unique<Bird>()); break;
            case 3: animals.push_back(std::make_unique<Fish>()); break;
            case 4: animals.push_back(std::make_unique<Rabbit>()); break;
        }
    }
    
    int total = 0;
    for (auto _ : state) {
        for (auto& animal : animals) {
            if (auto dog = dynamic_cast<Dog*>(animal.get())) {
                total += dog->bark();
            }
            else if (auto cat = dynamic_cast<Cat*>(animal.get())) {
                total += cat->meow();
            }
            else if (auto bird = dynamic_cast<Bird*>(animal.get())) {
                total += bird->chirp();
            }
            else if (auto fish = dynamic_cast<Fish*>(animal.get())) {
                total += fish->swim();
            }
            else if (auto rabbit = dynamic_cast<Rabbit*>(animal.get())) {
                total += rabbit->hop();
            }
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_DynamicCast);

static void BM_WorstCaseDynamicCast(benchmark::State& state) {
    std::vector<std::unique_ptr<Animal>> animals;
    
    for (int i = 0; i < 1000; ++i) {
        animals.push_back(std::make_unique<Rabbit>());
    }
    
    int total = 0;
    for (auto _ : state) {
        for (auto& animal : animals) {

            if (auto dog = dynamic_cast<Dog*>(animal.get())) {
                total += dog->bark();
            }
            else if (auto cat = dynamic_cast<Cat*>(animal.get())) {
                total += cat->meow();
            }
            else if (auto bird = dynamic_cast<Bird*>(animal.get())) {
                total += bird->chirp();
            }
            else if (auto fish = dynamic_cast<Fish*>(animal.get())) {
                total += fish->swim();
            }
            else if (auto rabbit = dynamic_cast<Rabbit*>(animal.get())) {
                total += rabbit->hop(); 
            }
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_WorstCaseDynamicCast);
