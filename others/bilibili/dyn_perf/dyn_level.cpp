#include <benchmark/benchmark.h>

class Base {
public:
    virtual ~Base() {}
};

class Level1 : public Base {
public:
    virtual ~Level1() {}
};

class Level2 : public Level1 {
public:
    virtual ~Level2() {}
};

class Level3 : public Level2 {
public:
    virtual ~Level3() {}
};

class Level4 : public Level3 {
public:
    virtual ~Level4() {}
};

class Level5 : public Level4 {
public:
    virtual ~Level5() {}
};

template <typename Level>
static void BM_DeepInheritance(benchmark::State& state) {
    Base* basePtr = new Level5();
    for (auto _ : state) {
        Level* derivedPtr = dynamic_cast<Level*>(basePtr);
        benchmark::DoNotOptimize(derivedPtr);
    }
    delete basePtr;
}

BENCHMARK_TEMPLATE(BM_DeepInheritance, Level1);
BENCHMARK_TEMPLATE(BM_DeepInheritance, Level2);
BENCHMARK_TEMPLATE(BM_DeepInheritance, Level3);
BENCHMARK_TEMPLATE(BM_DeepInheritance, Level4);
BENCHMARK_TEMPLATE(BM_DeepInheritance, Level5);
