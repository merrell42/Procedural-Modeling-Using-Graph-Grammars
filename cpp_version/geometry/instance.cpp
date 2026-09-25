#include "pch.h"
#include "instance.h"

void freeInstanceListMemory(InstanceList& instanceList) {
    if (instanceList.instances) {
        for (int i = 0; i < instanceList.count; i++) {
            if (instanceList.instances[i].assetId) {
                free((void*)instanceList.instances[i].assetId);
                instanceList.instances[i].assetId = nullptr;
            }
        }
        free(instanceList.instances);
        instanceList.instances = nullptr;
    }
    instanceList.count = 0;
}
