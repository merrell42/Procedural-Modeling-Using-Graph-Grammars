// Expand editor splices into exported graph topology with spliced flags.

const SPLIT_T_EPS = 1e-6;

function makeSplitKey(edgeIndex, t) {
    return `${edgeIndex}:${Math.round(t / SPLIT_T_EPS)}`;
}

function cloneExportGraph(source) {
    return {
        vertices: source.vertices.map(v => ({
            connections: [...(v.connections || [])],
            boundaryId: v.boundaryId || '',
            position: { x: v.position.x, y: v.position.y },
            spliced: false,
        })),
        edges: source.edges.map(e => ({
            start: e.start,
            end: e.end,
            spliced: false,
        })),
    };
}

function insertEdgeInCCWOrder(vertices, edges, vertexIndex, edgeIndex) {
    const vertex = vertices[vertexIndex];
    const edge = edges[edgeIndex];
    const otherVertexIndex = edge.start === vertexIndex ? edge.end : edge.start;
    const otherVertex = vertices[otherVertexIndex];
    const angle = Math.atan2(
        otherVertex.position.y - vertex.position.y,
        otherVertex.position.x - vertex.position.x
    );
    const normalizedAngle = angle < 0 ? angle + 2 * Math.PI : angle;

    let insertIndex = vertex.connections.length;
    for (let i = 0; i < vertex.connections.length; i++) {
        const existingEdge = edges[vertex.connections[i]];
        const existingOtherIndex = existingEdge.start === vertexIndex
            ? existingEdge.end
            : existingEdge.start;
        const existingOther = vertices[existingOtherIndex];
        const existingAngle = Math.atan2(
            existingOther.position.y - vertex.position.y,
            existingOther.position.x - vertex.position.x
        );
        const normalizedExistingAngle = existingAngle < 0 ? existingAngle + 2 * Math.PI : existingAngle;
        if (normalizedAngle < normalizedExistingAngle) {
            insertIndex = i;
            break;
        }
    }

    vertex.connections.splice(insertIndex, 0, edgeIndex);
}

function splitEdgeAtPoints(vertices, edges, edgeIndex, splitPoints, vertexBySplitKey) {
    const sorted = [...splitPoints].sort((a, b) => a.t - b.t);
    const deduped = [];
    for (const sp of sorted) {
        const last = deduped[deduped.length - 1];
        if (!last || sp.t - last.t > SPLIT_T_EPS) {
            deduped.push(sp);
        } else if (vertexBySplitKey.has(sp.key) && !vertexBySplitKey.has(last.key)) {
            vertexBySplitKey.set(sp.key, vertexBySplitKey.get(last.key));
        } else if (vertexBySplitKey.has(last.key) && !vertexBySplitKey.has(sp.key)) {
            vertexBySplitKey.set(sp.key, vertexBySplitKey.get(last.key));
        }
    }

    const edge = edges[edgeIndex];
    const startIdx = edge.start;
    const endIdx = edge.end;
    const startVertex = vertices[startIdx];
    const endVertex = vertices[endIdx];

    const startConnIdx = startVertex.connections.indexOf(edgeIndex);
    const endConnIdx = endVertex.connections.indexOf(edgeIndex);

    const chain = [startIdx];
    for (const sp of deduped) {
        let vIdx = vertexBySplitKey.get(sp.key);
        if (vIdx === undefined) {
            vIdx = vertices.length;
            vertices.push({
                connections: [],
                boundaryId: '',
                position: { x: sp.point.x, y: sp.point.y },
                spliced: true,
            });
            vertexBySplitKey.set(sp.key, vIdx);
        }
        chain.push(vIdx);
    }
    chain.push(endIdx);

    startVertex.connections.splice(startConnIdx, 1);
    endVertex.connections.splice(endConnIdx, 1);

    const segmentEdgeIndices = [];
    for (let i = 0; i < chain.length - 1; i++) {
        let eIdx;
        if (i === 0) {
            eIdx = edgeIndex;
            edges[eIdx] = { start: chain[i], end: chain[i + 1], spliced: false };
        } else {
            eIdx = edges.length;
            edges.push({ start: chain[i], end: chain[i + 1], spliced: false });
        }
        segmentEdgeIndices.push(eIdx);
    }

    startVertex.connections.splice(startConnIdx, 0, segmentEdgeIndices[0]);
    endVertex.connections.splice(
        endConnIdx,
        0,
        segmentEdgeIndices[segmentEdgeIndices.length - 1]
    );

    for (let i = 1; i < chain.length - 1; i++) {
        const vIdx = chain[i];
        insertEdgeInCCWOrder(vertices, edges, vIdx, segmentEdgeIndices[i - 1]);
        insertEdgeInCCWOrder(vertices, edges, vIdx, segmentEdgeIndices[i]);
    }
}

function addSplitPoint(splitsPerEdge, edgeIndex, t, point, key) {
    if (!splitsPerEdge.has(edgeIndex)) {
        splitsPerEdge.set(edgeIndex, []);
    }
    const list = splitsPerEdge.get(edgeIndex);
    if (!list.some(s => s.key === key)) {
        list.push({ t, point, key });
    }
}

function exportGraphWithSplices(graphTemplate) {
    const graph = cloneExportGraph(graphTemplate);
    const templateCopy = {
        vertices: graph.vertices,
        edges: graph.edges,
        edgeComponentIds: [],
        edgesByComponent: new Map(),
        splices: [],
    };
    updateTemplate(templateCopy);
    const splices = templateCopy.splices || [];

    if (splices.length === 0) {
        return graph;
    }

    const splitsPerEdge = new Map();
    for (const splice of splices) {
        const keyA = makeSplitKey(splice.edgeA, splice.tA);
        const keyB = makeSplitKey(splice.edgeB, splice.tB);
        addSplitPoint(splitsPerEdge, splice.edgeA, splice.tA, splice.pointA, keyA);
        addSplitPoint(splitsPerEdge, splice.edgeB, splice.tB, splice.pointB, keyB);
    }

    const vertexBySplitKey = new Map();
    const edgeIndices = [...splitsPerEdge.keys()].sort((a, b) => a - b);
    for (const edgeIndex of edgeIndices) {
        splitEdgeAtPoints(
            graph.vertices,
            graph.edges,
            edgeIndex,
            splitsPerEdge.get(edgeIndex),
            vertexBySplitKey
        );
    }

    for (const splice of splices) {
        const keyA = makeSplitKey(splice.edgeA, splice.tA);
        const keyB = makeSplitKey(splice.edgeB, splice.tB);
        const v1 = vertexBySplitKey.get(keyA);
        const v2 = vertexBySplitKey.get(keyB);
        const eIdx = graph.edges.length;
        graph.edges.push({ start: v1, end: v2, spliced: true });
        insertEdgeInCCWOrder(graph.vertices, graph.edges, v1, eIdx);
        insertEdgeInCCWOrder(graph.vertices, graph.edges, v2, eIdx);
    }

    return graph;
}
