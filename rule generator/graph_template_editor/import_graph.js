// Collapse exported graph topology (spliced flags) back into editor format.
// Editor splices are recomputed by updateTemplate after import.

function graphHasSplicedTopology(graph) {
    return graph.vertices.some(v => v.spliced) || graph.edges.some(e => e.spliced);
}

function removeEdgeAtIndex(vertices, edges, edgeIndex) {
    const edge = edges[edgeIndex];
    if (!edge) return;

    const vStart = vertices[edge.start];
    const vEnd = vertices[edge.end];
    vStart.connections = vStart.connections.filter(c => c !== edgeIndex);
    vEnd.connections = vEnd.connections.filter(c => c !== edgeIndex);
    edges.splice(edgeIndex, 1);

    for (const vertex of vertices) {
        for (let i = 0; i < vertex.connections.length; i++) {
            if (vertex.connections[i] > edgeIndex) {
                vertex.connections[i]--;
            }
        }
    }
}

function removeVertexAtIndex(vertices, edges, vertexIndex) {
    vertices.splice(vertexIndex, 1);
    for (const edge of edges) {
        if (edge.start > vertexIndex) edge.start--;
        if (edge.end > vertexIndex) edge.end--;
    }
}

function otherEndpoint(edge, vertexIndex) {
    return edge.start === vertexIndex ? edge.end : edge.start;
}

function collapseSplicedVertex(vertices, edges, vertexIndex) {
    const vertex = vertices[vertexIndex];
    if (!vertex || !vertex.spliced) {
        return false;
    }

    const segmentEdgeIndices = vertex.connections.filter(
        edgeIndex => edgeIndex >= 0 && edgeIndex < edges.length && !edges[edgeIndex].spliced
    );
    if (segmentEdgeIndices.length !== 2) {
        return false;
    }

    const [keepEdgeIndex, removeEdgeIndex] = segmentEdgeIndices[0] < segmentEdgeIndices[1]
        ? segmentEdgeIndices
        : [segmentEdgeIndices[1], segmentEdgeIndices[0]];
    const keepEdge = edges[keepEdgeIndex];
    const removeEdge = edges[removeEdgeIndex];

    const endpointA = otherEndpoint(keepEdge, vertexIndex);
    const endpointB = otherEndpoint(removeEdge, vertexIndex);

    keepEdge.start = endpointA;
    keepEdge.end = endpointB;
    keepEdge.spliced = false;

    const endpointBVertex = vertices[endpointB];
    for (let i = 0; i < endpointBVertex.connections.length; i++) {
        if (endpointBVertex.connections[i] === removeEdgeIndex) {
            endpointBVertex.connections[i] = keepEdgeIndex;
        } else if (endpointBVertex.connections[i] > removeEdgeIndex) {
            endpointBVertex.connections[i]--;
        }
    }

    removeEdgeAtIndex(vertices, edges, removeEdgeIndex);

    removeVertexAtIndex(vertices, edges, vertexIndex);
    return true;
}

function importGraphWithSplices(exportedGraph) {
    const vertices = exportedGraph.vertices.map(v => ({
        connections: [...(v.connections || [])],
        boundaryId: v.boundaryId || '',
        position: { x: v.position.x, y: v.position.y },
        spliced: v.spliced ?? false,
    }));
    const edges = exportedGraph.edges.map(e => ({
        start: e.start,
        end: e.end,
        spliced: e.spliced ?? false,
    }));

    if (!graphHasSplicedTopology({ vertices, edges })) {
        return { vertices, edges };
    }

    const splicedEdgeIndices = edges
        .map((edge, index) => (edge.spliced ? index : -1))
        .filter(index => index >= 0)
        .sort((a, b) => b - a);
    for (const edgeIndex of splicedEdgeIndices) {
        removeEdgeAtIndex(vertices, edges, edgeIndex);
    }

    let collapsed = true;
    while (collapsed) {
        collapsed = false;
        const splicedVertexIndices = vertices
            .map((vertex, index) => (vertex.spliced ? index : -1))
            .filter(index => index >= 0)
            .sort((a, b) => b - a);
        for (const vertexIndex of splicedVertexIndices) {
            if (collapseSplicedVertex(vertices, edges, vertexIndex)) {
                collapsed = true;
            }
        }
    }

    for (const vertex of vertices) {
        vertex.spliced = false;
    }
    for (const edge of edges) {
        edge.spliced = false;
    }

    return { vertices, edges };
}
