#include "GrainPool.h"

Grain* GrainPool::getNextFreeGrain()
{
    for (auto& grain : grains)
        if (! grain.isActive)
            return &grain;

    return nullptr;
}

void GrainPool::returnGrain (Grain* grain)
{
    if (grain == nullptr)
        return;

    grain->isActive = false;
    grain->currentSample = 0;
}
