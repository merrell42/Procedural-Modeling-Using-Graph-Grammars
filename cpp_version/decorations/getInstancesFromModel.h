#pragma once
#include "../geometry/instance.h"
#include "../geometry/mesh.h"

class Model;

InstanceList getInstancesFromModel(Model* model);
void appendExtrusions(MeshCpp& mesh, Model* model);
void markOutputDirty();
