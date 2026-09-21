#pragma once

#include "matrix4.h"

struct Instance {
    const char* assetId;
    Matrix4 transform;
};

struct InstanceList {
    Instance* instances;
    int count;
};

void freeInstanceListMemory(InstanceList& instanceList);
