#include "pch.h"
#include "State.h"

int VertexState::GetConnectionIndex(int connectionIndex) {
	int n = (int)type->getHalfEdgeTypes().size();
	return (connectionIndex - edge0 + n) % n;
}

string VertexState::GetConnectionId(int connectionIndex) {
	return type->getHalfEdgeTypes()[GetConnectionIndex(connectionIndex)].getId();
}
