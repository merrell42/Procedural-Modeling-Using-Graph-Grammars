// Minimum distance from an edge endpoint to where a splice attaches on that edge.
const OFFSET_FROM_ENDPOINT = 40;

// Edges belong to the same connected component when they share a vertex
// (directly or through a chain of edges). Recomputed whenever the graph changes.
function updateEdgeConnectedComponents(graphTemplate) {
    const n = graphTemplate.edges.length;
    if (n === 0) {
        graphTemplate.edgeComponentIds = [];
        graphTemplate.edgesByComponent = new Map();
        return;
    }

    const parent = Array.from({ length: n }, (_, i) => i);

    function find(i) {
        while (parent[i] !== i) {
            parent[i] = parent[parent[i]];
            i = parent[i];
        }
        return i;
    }

    function union(a, b) {
        const ra = find(a);
        const rb = find(b);
        if (ra !== rb) parent[ra] = rb;
    }

    for (const vertex of graphTemplate.vertices) {
        const edges = vertex.connections;
        for (let i = 1; i < edges.length; i++) {
            union(edges[0], edges[i]);
        }
    }

    const rootToId = new Map();
    const edgeComponentIds = new Array(n);
    const edgesByComponent = new Map();
    for (let e = 0; e < n; e++) {
        const root = find(e);
        if (!rootToId.has(root)) rootToId.set(root, rootToId.size);
        const componentId = rootToId.get(root);
        edgeComponentIds[e] = componentId;
        if (!edgesByComponent.has(componentId)) {
            edgesByComponent.set(componentId, []);
        }
        edgesByComponent.get(componentId).push(e);
    }

    graphTemplate.edgeComponentIds = edgeComponentIds;
    graphTemplate.edgesByComponent = edgesByComponent;
}

function getEdgeEndpoints(graphTemplate, edgeIndex) {
    const edge = graphTemplate.edges[edgeIndex];
    const vStart = graphTemplate.vertices[edge.start];
    const vEnd = graphTemplate.vertices[edge.end];
    return { a: vStart.position, b: vEnd.position };
}

function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
}

function dot(ax, ay, bx, by) {
    return ax * bx + ay * by;
}

function moveSpliceAwayFromEndpoints(point, endpointA, endpointB) {
    const dx = endpointB.x - endpointA.x;
    const dy = endpointB.y - endpointA.y;
    const lengthSq = dx * dx + dy * dy;
    const length = Math.sqrt(lengthSq);
    // If the segment is shorter than twice the offset, return the midpoint.
    if (length < 2 * OFFSET_FROM_ENDPOINT) {
        return {
            x: (endpointA.x + endpointB.x) / 2,
            y: (endpointA.y + endpointB.y) / 2,
        };
    }

    let t = dot(point.x - endpointA.x, point.y - endpointA.y, dx, dy) / lengthSq;
    t = clamp(t, OFFSET_FROM_ENDPOINT / length, 1 - OFFSET_FROM_ENDPOINT / length);
    return { x: endpointA.x + t * dx, y: endpointA.y + t * dy };
}

// Closest points between two line segments (Ericson, Real-Time Collision Detection).
function closestPointsOnSegments(a, b, c, d) {
    const ux = b.x - a.x;
    const uy = b.y - a.y;
    const vx = d.x - c.x;
    const vy = d.y - c.y;
    const wx = a.x - c.x;
    const wy = a.y - c.y;

    const aCoeff = dot(ux, uy, ux, uy);
    const bCoeff = dot(ux, uy, vx, vy);
    const cCoeff = dot(vx, vy, vx, vy);
    const dCoeff = dot(ux, uy, wx, wy);
    const eCoeff = dot(vx, vy, wx, wy);
    const denom = aCoeff * cCoeff - bCoeff * bCoeff;

    let sN;
    let sD = denom;
    let tN;
    let tD = denom;

    if (denom <= 1e-10) {
        sN = 0;
        sD = 1;
        tN = eCoeff;
        tD = cCoeff;
    } else {
        sN = bCoeff * eCoeff - cCoeff * dCoeff;
        tN = aCoeff * eCoeff - bCoeff * dCoeff;
        if (sN < 0) {
            sN = 0;
            tN = eCoeff;
            tD = cCoeff;
        } else if (sN > sD) {
            sN = sD;
            tN = eCoeff + bCoeff;
            tD = cCoeff;
        }
    }

    if (tN < 0) {
        tN = 0;
        if (-dCoeff < 0) {
            sN = 0;
        } else if (-dCoeff > aCoeff) {
            sN = sD;
        } else {
            sN = -dCoeff;
            sD = aCoeff;
        }
    } else if (tN > tD) {
        tN = tD;
        if (-dCoeff + bCoeff < 0) {
            sN = 0;
        } else if (-dCoeff + bCoeff > aCoeff) {
            sN = sD;
        } else {
            sN = -dCoeff + bCoeff;
            sD = aCoeff;
        }
    }

    const sc = Math.abs(sN) <= 1e-10 ? 0 : sN / sD;
    const tc = Math.abs(tN) <= 1e-10 ? 0 : tN / tD;

    const pointA = { x: a.x + sc * ux, y: a.y + sc * uy };
    const pointB = { x: c.x + tc * vx, y: c.y + tc * vy };
    return {
        dist: Math.hypot(pointA.x - pointB.x, pointA.y - pointB.y),
        pointA,
        pointB,
    };
}

function parametricTOnSegment(point, endpointA, endpointB) {
    const dx = endpointB.x - endpointA.x;
    const dy = endpointB.y - endpointA.y;
    const lengthSq = dx * dx + dy * dy;
    return dot(point.x - endpointA.x, point.y - endpointA.y, dx, dy) / lengthSq;
}

function minDistanceBetweenEdgeSets(graphTemplate, edgesA, edgesB) {
    let closest = null;
    for (const edgeA of edgesA) {
        const segA = getEdgeEndpoints(graphTemplate, edgeA);
        for (const edgeB of edgesB) {
            const segB = getEdgeEndpoints(graphTemplate, edgeB);
            const result = closestPointsOnSegments(segA.a, segA.b, segB.a, segB.b);
            if (!closest || result.dist < closest.dist) {
                closest = {
                    dist: result.dist,
                    pointA: result.pointA,
                    pointB: result.pointB,
                    segA,
                    segB,
                    edgeA,
                    edgeB,
                };
            }
        }
    }
    if (!closest) {
        return null;
    }
    const pointA = moveSpliceAwayFromEndpoints(closest.pointA, closest.segA.a, closest.segA.b);
    const pointB = moveSpliceAwayFromEndpoints(closest.pointB, closest.segB.a, closest.segB.b);
    return {
        dist: closest.dist,
        pointA,
        pointB,
        edgeA: closest.edgeA,
        edgeB: closest.edgeB,
        tA: parametricTOnSegment(pointA, closest.segA.a, closest.segA.b),
        tB: parametricTOnSegment(pointB, closest.segB.a, closest.segB.b),
    };
}

// Connect all edge components with gray splices using a minimum spanning tree
// on component-to-component distances (closest edge pair per component pair).
function updateSplices(graphTemplate) {
    const numComponents = graphTemplate.edgesByComponent.size;
    if (numComponents <= 1) {
        graphTemplate.splices = [];
        return;
    }

    const componentIds = [...graphTemplate.edgesByComponent.keys()];
    const candidateSplices = [];

    for (let i = 0; i < componentIds.length; i++) {
        for (let j = i + 1; j < componentIds.length; j++) {
            const closest = minDistanceBetweenEdgeSets(
                graphTemplate,
                graphTemplate.edgesByComponent.get(componentIds[i]),
                graphTemplate.edgesByComponent.get(componentIds[j])
            );
            if (closest) {
                candidateSplices.push({
                    componentA: componentIds[i],
                    componentB: componentIds[j],
                    ...closest,
                });
            }
        }
    }

    candidateSplices.sort((a, b) => a.dist - b.dist);

    const parent = new Map(componentIds.map(id => [id, id]));
    function find(id) {
        let root = id;
        while (parent.get(root) !== root) {
            root = parent.get(root);
        }
        let current = id;
        while (parent.get(current) !== root) {
            const next = parent.get(current);
            parent.set(current, root);
            current = next;
        }
        return root;
    }

    function union(a, b) {
        parent.set(find(a), find(b));
    }

    const splices = [];
    for (const candidate of candidateSplices) {
        const { componentA, componentB, dist, ...splice } = candidate;
        if (find(componentA) === find(componentB)) {
            continue;
        }
        union(componentA, componentB);
        splices.push({ ...splice });
    }

    graphTemplate.splices = splices;
}

function updateTemplate(graphTemplate) {
    updateEdgeConnectedComponents(graphTemplate);
    updateSplices(graphTemplate);
}
