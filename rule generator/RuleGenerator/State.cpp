#include "pch.h"
#include "State.h"

int VertexState::GetConnectionIndex(int connectionIndex) const {
	int n = (int)type->getHalfEdgeTypes().size();
	return (connectionIndex - edge0 + n) % n;
}

string VertexState::GetConnectionId(int connectionIndex) const {
	return type->getHalfEdgeTypes()[GetConnectionIndex(connectionIndex)].getId();
}
