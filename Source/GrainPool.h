#pragma once

#include <array>

#include "Grain.h"

class GrainPool
{
public:
    static constexpr size_t poolSize = 128;

    Grain* getNextFreeGrain();
    void returnGrain (Grain* grain);

    std::array<Grain, poolSize>& getGrains() noexcept { return grains; }
    const std::array<Grain, poolSize>& getGrains() const noexcept { return grains; }

private:
    std::array<Grain, poolSize> grains {};
};
