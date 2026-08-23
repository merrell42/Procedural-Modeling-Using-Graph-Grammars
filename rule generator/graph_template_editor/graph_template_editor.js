// Vertex rendering settings (shared constants).
const VERTEX_RADIUS = 12;
const VERTEX_COLOR = '#2196F3';
const VERTEX_BORDER_COLOR = '#1976D2';
const PREVIEW_COLOR = 'rgba(33, 150, 243, 0.3)';
const PREVIEW_BORDER_COLOR = 'rgba(25, 118, 210, 0.4)';

// Undo/redo history. A single global stack captures snapshots of *both*
// editors plus their selection, since boundary edits and library navigation
// mutate both canvases atomically.
const History = {
    past: [],
    future: [],
    cap: 100,
    suspended: false,

    snapshot() {
        return {
            e1: structuredClone(window.editor1.graphTemplate),
            e2: structuredClone(window.editor2.graphTemplate),
            sel1: window.editor1.selectedVertexIndex,
            sel2: window.editor2.selectedVertexIndex,
            library: structuredClone(window.library || []),
            activeIdx: typeof window.activeIdx === 'number' ? window.activeIdx : -1,
        };
    },

    // Call BEFORE a mutation. Duplicates on no-op clicks are harmless.
    commit() {
        if (this.suspended) return;
        if (!window.editor1 || !window.editor2) return;
        this.past.push(this.snapshot());
        if (this.past.length > this.cap) this.past.shift();
        this.future.length = 0;
        updateHistoryButtons();
    },

    undo() {
        if (this.past.length === 0) return;
        this.future.push(this.snapshot());
        this.applySnapshot(this.past.pop());
        updateHistoryButtons();
    },

    redo() {
        if (this.future.length === 0) return;
        this.past.push(this.snapshot());
        this.applySnapshot(this.future.pop());
        updateHistoryButtons();
    },

    applySnapshot(snap) {
        this.suspended = true;
        try {
            window.editor1.graphTemplate = structuredClone(snap.e1);
            window.editor2.graphTemplate = structuredClone(snap.e2);
            window.editor1.selectedVertexIndex = clampSelection(snap.sel1, snap.e1.vertices.length);
            window.editor2.selectedVertexIndex = clampSelection(snap.sel2, snap.e2.vertices.length);
            window.library = structuredClone(snap.library);
            window.activeIdx = clampActive(snap.activeIdx, window.library.length);
            window.editor1.refreshTemplate();
            window.editor2.refreshTemplate();
            updateLibraryUI();
        } finally {
            this.suspended = false;
        }
    },
};

function clampSelection(sel, n) {
    if (sel === null || sel === undefined) return null;
    return sel >= 0 && sel < n ? sel : null;
}

function clampActive(idx, n) {
    if (n === 0) return -1;
    if (idx < 0) return -1;
    if (idx >= n) return n - 1;
    return idx;
}

function updateHistoryButtons() {
    const undoBtn = document.getElementById('undoBtn');
    const redoBtn = document.getElementById('redoBtn');
    if (undoBtn) undoBtn.disabled = History.past.length === 0;
    if (redoBtn) redoBtn.disabled = History.future.length === 0;
}

function isTypingInTextInput(el) {
    if (!el) return false;
    const tag = el.tagName;
    if (tag === 'INPUT' || tag === 'TEXTAREA') return true;
    if (el.isContentEditable) return true;
    return false;
}

function cloneGraphData(graph) {
    return {
        vertices: graph.vertices.map(v => ({
            connections: v.connections || [],
            boundaryId: v.boundaryId || '',
            position: { ...v.position },
            spliced: v.spliced ?? false,
        })),
        edges: graph.edges.map(e => ({
            start: e.start,
            end: e.end,
            spliced: e.spliced ?? false,
        })),
    };
}

// TemplateEditor class to manage a single graph template.
class TemplateEditor {
    constructor(templateId) {
        this.templateId = templateId;
        
        // Graph template data structure.
        this.graphTemplate = {
            vertices: [],
            edges: [],
            edgeComponentIds: [],
            edgesByComponent: new Map(),
            splices: [],
        };
        
        // DOM element references.
        this.canvas = document.getElementById(`canvas${templateId}`);
        this.ctx = this.canvas.getContext('2d');
        
        // Track mouse position for preview.
        this.hoverPosition = null;
        // Track edge creation mode.
        this.selectedVertexIndex = null;
        this.hoveredVertexIndex = null;
        // Track clicks for double-click detection.
        this.clickTimeout = null;
        this.lastClickTime = 0;
        this.lastClickPosition = null;
        
        // Bind methods to preserve 'this' context.
        this.handleCanvasClick = this.handleCanvasClick.bind(this);
        this.handleCanvasDoubleClick = this.handleCanvasDoubleClick.bind(this);
        this.handleCanvasMouseMove = this.handleCanvasMouseMove.bind(this);
        this.handleCanvasMouseLeave = this.handleCanvasMouseLeave.bind(this);
    }
    
    // Generate a unique boundary ID.
    generateboundaryId() {
        return Math.random().toString(36).substring(2, 15) + Math.random().toString(36).substring(2, 15);
    }
    
    // Initialize canvas.
    init() {
        // Make canvas responsive to fit container.
        this.resizeCanvas();
        window.addEventListener('resize', () => this.resizeCanvas());
        
        this.canvas.addEventListener('click', this.handleCanvasClick);
        this.canvas.addEventListener('dblclick', this.handleCanvasDoubleClick);
        this.canvas.addEventListener('mousemove', this.handleCanvasMouseMove);
        this.canvas.addEventListener('mouseleave', this.handleCanvasMouseLeave);
        
        
        this.refreshTemplate();
    }
    
    // Resize canvas to fit container.
    resizeCanvas() {
        const container = this.canvas.parentElement;
        const containerWidth = container.clientWidth;
        const containerHeight = container.clientHeight;
        
        // Set canvas to fill container.
        this.canvas.width = containerWidth;
        this.canvas.height = containerHeight;
        
        // Set display size to match internal size exactly (no CSS scaling).
        this.canvas.style.setProperty('width', this.canvas.width + 'px', 'important');
        this.canvas.style.setProperty('height', this.canvas.height + 'px', 'important');
        this.canvas.style.setProperty('max-width', 'none', 'important');
        this.canvas.style.setProperty('max-height', 'none', 'important');
        
        // Redraw after resize.
        this.refreshTemplate();
    }
    
    // Graph space has +Y up (same as primitives). Canvas pixels have +Y down.
    toScreen(x, y) {
        return { x, y: this.canvas.height - y };
    }

    fromScreen(x, y) {
        return { x, y: this.canvas.height - y };
    }

    // Check if a point is within a vertex's radius.
    isPointInVertex(x, y, vertex) {
        const dx = x - vertex.position.x;
        const dy = y - vertex.position.y;
        return dx * dx + dy * dy <= VERTEX_RADIUS * VERTEX_RADIUS;
    }
    
    // Find vertex at given position.
    findVertexAt(x, y) {
        for (let i = 0; i < this.graphTemplate.vertices.length; i++) {
            if (this.isPointInVertex(x, y, this.graphTemplate.vertices[i])) {
                return i;
            }
        }
        return null;
    }
    
    // Update boundaryId property for a specific vertex based on connection count.
    updateOnBoundaryForVertex(vertex) {
        const wasBoundary = vertex.boundaryId !== '';
        const shouldBeBoundary = vertex.connections.length === 1;
        
        if (shouldBeBoundary && !wasBoundary) {
            // Changed from false to true - create unique ID and add vertex in other template.
            vertex.boundaryId = this.generateboundaryId();
            this.addBoundaryVertexInOtherTemplate(vertex.position, vertex.boundaryId);
        } else if (!shouldBeBoundary && wasBoundary) {
            // Changed from true to false - remove vertex in other template and clear ID.
            this.removeBoundaryVertexInOtherTemplate(vertex.boundaryId);
            vertex.boundaryId = '';
        }
    }
    
    // Find an edge between the two given vertex indices.
    findEdgeBetween(indexA, indexB) {
        return this.graphTemplate.edges.find(e =>
            (e.start === indexA && e.end === indexB) ||
            (e.start === indexB && e.end === indexA)
        );
    }

    // Remove an edge and renumber remaining connection indices.
    removeEdgeFromGraph(edgeIndex) {
        const edge = this.graphTemplate.edges[edgeIndex];
        if (!edge) return;

        const vStart = this.graphTemplate.vertices[edge.start];
        const vEnd = this.graphTemplate.vertices[edge.end];
        vStart.connections.splice(vStart.connections.indexOf(edgeIndex), 1);
        vEnd.connections.splice(vEnd.connections.indexOf(edgeIndex), 1);
        this.graphTemplate.edges.splice(edgeIndex, 1);

        for (const vertex of this.graphTemplate.vertices) {
            for (let i = 0; i < vertex.connections.length; i++) {
                if (vertex.connections[i] > edgeIndex) {
                    vertex.connections[i]--;
                }
            }
        }
        this.updateOnBoundaryForVertex(vStart);
        this.updateOnBoundaryForVertex(vEnd);
    }

    // Insert edge in counter-clockwise order around a vertex.
    insertEdgeInOrder(vertexIndex, edgeIndex, newAngle) {
        const vertex = this.graphTemplate.vertices[vertexIndex];
        const normalizedAngle = newAngle < 0 ? newAngle + 2 * Math.PI : newAngle;
        let insertIndex = vertex.connections.length;

        for (let i = 0; i < vertex.connections.length; i++) {
            const existingEdge = this.graphTemplate.edges[vertex.connections[i]];
            const otherVertexIndex = existingEdge.start === vertexIndex
                ? existingEdge.end
                : existingEdge.start;
            const otherVertex = this.graphTemplate.vertices[otherVertexIndex];
            // Graph +Y is up, so increasing atan2 is CCW, matching primitives.
            const existingAngle = Math.atan2(
                otherVertex.position.y - vertex.position.y,
                otherVertex.position.x - vertex.position.x
            );
            const normalizedExistingAngle = existingAngle < 0 ? existingAngle + 2 * Math.PI : existingAngle;
            if (normalizedAngle < normalizedExistingAngle) {
                insertIndex = i;
                break;
            }
        }

        vertex.connections.splice(insertIndex, 0, edgeIndex);
    }
    
    // Get canvas coordinates from mouse event.
    getCanvasCoordinates(event) {
        const rect = this.canvas.getBoundingClientRect();
        
        // Calculate position relative to canvas top-left corner.
        const clickX = event.clientX - rect.left;
        const clickY = event.clientY - rect.top;
        
        // Clamp coordinates to canvas bounds (allow clicks slightly outside to still work).
        const clampedX = Math.max(0, Math.min(rect.width - 1, clickX));
        const clampedY = Math.max(0, Math.min(rect.height - 1, clickY));
        
        // Calculate scale factors (should be 1:1 if canvas display size matches internal size).
        const scaleX = this.canvas.width / rect.width;
        const scaleY = this.canvas.height / rect.height;
        
        // Canvas pixels are Y-down; convert to graph space (+Y up).
        return this.fromScreen(clampedX * scaleX, clampedY * scaleY);
    }
    
    // Handle canvas mouse move for preview.
    handleCanvasMouseMove(event) {
        const coords = this.getCanvasCoordinates(event);
        if (!coords) {
            this.hoverPosition = null;
            this.hoveredVertexIndex = null;
            this.canvas.style.cursor = 'url("data:image/svg+xml,%3Csvg xmlns=\'http://www.w3.org/2000/svg\' width=\'20\' height=\'20\' viewBox=\'0 0 20 20\'%3E%3Cline x1=\'10\' y1=\'0\' x2=\'10\' y2=\'20\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3Cline x1=\'0\' y1=\'10\' x2=\'20\' y2=\'10\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3C/svg%3E") 10 10, crosshair';
            this.drawVertices();
            return;
        }
        const { x, y } = coords;
        
        this.hoverPosition = { x: x, y: y };
        
        // Check if hovering over a vertex.
        const vertexIndex = this.findVertexAt(x, y);
        if (vertexIndex !== null) {
            this.hoveredVertexIndex = vertexIndex;
            this.canvas.style.cursor = 'pointer';
        } else {
            this.hoveredVertexIndex = null;
            this.canvas.style.cursor = 'url("data:image/svg+xml,%3Csvg xmlns=\'http://www.w3.org/2000/svg\' width=\'20\' height=\'20\' viewBox=\'0 0 20 20\'%3E%3Cline x1=\'10\' y1=\'0\' x2=\'10\' y2=\'20\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3Cline x1=\'0\' y1=\'10\' x2=\'20\' y2=\'10\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3C/svg%3E") 10 10, crosshair';
        }
        
        this.drawVertices();
    }
    
    // Handle canvas mouse leave to clear preview.
    handleCanvasMouseLeave() {
        this.hoverPosition = null;
        this.hoveredVertexIndex = null;
        this.canvas.style.cursor = 'url("data:image/svg+xml,%3Csvg xmlns=\'http://www.w3.org/2000/svg\' width=\'20\' height=\'20\' viewBox=\'0 0 20 20\'%3E%3Cline x1=\'10\' y1=\'0\' x2=\'10\' y2=\'20\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3Cline x1=\'0\' y1=\'10\' x2=\'20\' y2=\'10\' stroke=\'rgba(150,150,150,0.6)\' stroke-width=\'1\'/%3E%3C/svg%3E") 10 10, crosshair';
        this.drawVertices();
    }
    
    // Handle canvas double-click to toggle boundaryId.
    handleCanvasDoubleClick(event) {
        // Clear any pending single click.
        if (this.clickTimeout) {
            clearTimeout(this.clickTimeout);
            this.clickTimeout = null;
        }
        
        const coords = this.getCanvasCoordinates(event);
        if (!coords) return;
        const { x, y } = coords;
        
        // Check if double-clicking on a vertex.
        const vertexIndex = this.findVertexAt(x, y);
        
        if (vertexIndex !== null) {
            const vertex = this.graphTemplate.vertices[vertexIndex];
            const wasBoundary = vertex.boundaryId !== '';
            const position = { x: vertex.position.x, y: vertex.position.y };
            // Snapshot before mutating; boundary toggles touch the other editor too.
            History.commit();

            // Toggle boundaryId.
            if (wasBoundary) {
                // Set to empty string.
                this.removeBoundaryVertexInOtherTemplate(vertex.boundaryId);
                vertex.boundaryId = '';
                this.refreshTemplate();
            } else if (vertex.connections.length === 1) {
                // Only allow setting to true if vertex has exactly one edge.
                vertex.boundaryId = this.generateboundaryId();
                this.refreshTemplate();
                
                // Add boundary vertex at same position in other template.
                this.addBoundaryVertexInOtherTemplate(position, vertex.boundaryId);
            } else {
            }
        }
    }
    
    // Handle canvas click to add vertex or create edge.
    handleCanvasClick(event) {
        const coords = this.getCanvasCoordinates(event);
        if (!coords) return;
        const { x, y } = coords;
        
        const currentTime = Date.now();
        const currentPosition = { x, y };
        
        // Check if this might be a double-click (same position, within 300ms).
        const isPotentialDoubleClick = 
            currentTime - this.lastClickTime < 300 &&
            this.lastClickPosition &&
            Math.abs(currentPosition.x - this.lastClickPosition.x) < 5 &&
            Math.abs(currentPosition.y - this.lastClickPosition.y) < 5;
        
        if (isPotentialDoubleClick) {
            // Wait a bit to see if double-click fires.
            this.lastClickTime = currentTime;
            this.lastClickPosition = currentPosition;
            return;
        }
        
        // Clear any pending timeout.
        if (this.clickTimeout) {
            clearTimeout(this.clickTimeout);
        }
        
        // Store click info.
        this.lastClickTime = currentTime;
        this.lastClickPosition = currentPosition;
        
        // Delay the click handler to allow double-click detection.
        this.clickTimeout = setTimeout(() => {
            this.processCanvasClick(x, y);
            this.clickTimeout = null;
        }, 200);
    }
    
    // Process the actual click action.
    processCanvasClick(x, y) {
        // Snapshot before any potential mutation. Pure-deselect clicks will
        // produce a no-op duplicate snapshot, which is harmless.
        History.commit();

        // Check if clicking on a vertex.
        const vIndex1 = this.selectedVertexIndex;
        const vIndex2 = this.findVertexAt(x, y);
        
        if (vIndex2 !== null) {
            // Clicked on a vertex - handle edge creation.
            if (this.selectedVertexIndex === null) {
                // First vertex selected.
                this.selectedVertexIndex = vIndex2;
            } else if (this.selectedVertexIndex === vIndex2) {
                // Clicked the same vertex - deselect.
                this.selectedVertexIndex = null;
            } else {
                // Second vertex selected - create edge.
                const v1 = this.graphTemplate.vertices[vIndex1];
                const v2 = this.graphTemplate.vertices[vIndex2];
                
                // Check if edge already exists (vertices share a common edge index).
                if (!this.findEdgeBetween(vIndex1, vIndex2)) {
                    // Add edge indices (using current numEdges as the edge index).
                    const edgeIndex = this.graphTemplate.edges.length;
                    this.graphTemplate.edges.push({
                        start: vIndex1,
                        end: vIndex2,
                    });

                    // Calculate angle from v1 to v2 for counter-clockwise ordering.
                    const angle1 = Math.atan2(v2.position.y - v1.position.y, v2.position.x - v1.position.x);
                    this.insertEdgeInOrder(vIndex1, edgeIndex, angle1);

                    // Calculate angle from v2 to v1 for counter-clockwise ordering.
                    const angle2 = Math.atan2(v1.position.y - v2.position.y, v1.position.x - v2.position.x);
                    this.insertEdgeInOrder(vIndex2, edgeIndex, angle2);

                    // Update onBoundary for the two vertices involved in the edge.
                    this.updateOnBoundaryForVertex(v1);
                    this.updateOnBoundaryForVertex(v2);
                }
                
                this.selectedVertexIndex = null;
            }
            this.refreshTemplate();
        } else {
            // Clicked on empty space.
            if (this.selectedVertexIndex === null) {
                // Not in edge creation mode - add new vertex.
                const vertex = {
                    connections: [],
                    boundaryId: '',
                    position: { x: x, y: y }
                };
                
                this.graphTemplate.vertices.push(vertex);
                this.refreshTemplate();
            } else {
                // In edge creation mode - add vertex and connect it to selected vertex.
                const newVertex = {
                    connections: [],
                    boundaryId: '',
                    position: { x: x, y: y }
                };
                const newVertexIndex = this.graphTemplate.vertices.length;
                this.graphTemplate.vertices.push(newVertex);
                
                // Create edge between selected vertex and new vertex.
                const v1 = this.graphTemplate.vertices[vIndex1];
                const edgeIndex = this.graphTemplate.edges.length;
                this.graphTemplate.edges.push({
                    start: vIndex1,
                    end: newVertexIndex,
                });

                // Calculate angle from v1 to new vertex for counter-clockwise ordering.
                const angle1 = Math.atan2(newVertex.position.y - v1.position.y, newVertex.position.x - v1.position.x);
                this.insertEdgeInOrder(vIndex1, edgeIndex, angle1);

                // Calculate angle from new vertex to v1 for counter-clockwise ordering.
                const angle2 = Math.atan2(v1.position.y - newVertex.position.y, v1.position.x - newVertex.position.x);
                this.insertEdgeInOrder(newVertexIndex, edgeIndex, angle2);
                
                // Update onBoundary for the two vertices involved in the edge.
                this.updateOnBoundaryForVertex(v1);
                this.updateOnBoundaryForVertex(newVertex);

                this.selectedVertexIndex = null;
                this.refreshTemplate();
            }
        }
    }
    
    // Draw all vertices on canvas.
    drawVertices() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Draw edges.
        for (const edge of this.graphTemplate.edges) {
            const vStart = this.graphTemplate.vertices[edge.start];
            const vEnd = this.graphTemplate.vertices[edge.end];
            if (!vStart || !vEnd) continue;
            const start = this.toScreen(vStart.position.x, vStart.position.y);
            const end = this.toScreen(vEnd.position.x, vEnd.position.y);
            this.ctx.beginPath();
            this.ctx.moveTo(start.x, start.y);
            this.ctx.lineTo(end.x, end.y);
            this.ctx.strokeStyle = '#0066FF';
            this.ctx.lineWidth = 1;
            this.ctx.stroke();
        }

        // Draw splices connecting disconnected edge components.
        this.ctx.setLineDash([4, 4]);
        for (const splice of this.graphTemplate.splices || []) {
            const a = this.toScreen(splice.pointA.x, splice.pointA.y);
            const b = this.toScreen(splice.pointB.x, splice.pointB.y);
            this.ctx.beginPath();
            this.ctx.moveTo(a.x, a.y);
            this.ctx.lineTo(b.x, b.y);
            this.ctx.strokeStyle = '#888888';
            this.ctx.lineWidth = 1;
            this.ctx.stroke();
        }
        this.ctx.setLineDash([]);
        
        // Draw existing vertices.
        this.graphTemplate.vertices.forEach((vertex, index) => {
            const { x, y } = this.toScreen(vertex.position.x, vertex.position.y);
            
            // Determine vertex color based on state.
            let fillColor = VERTEX_COLOR;
            let strokeColor = VERTEX_BORDER_COLOR;
            
            if (index === this.selectedVertexIndex) {
                    fillColor = '#BB66FF'; // Brighter purple-blue for selected.
                strokeColor = '#9933FF';
            } else if (index === this.hoveredVertexIndex) {
                fillColor = '#BB66FF'; // Brighter purple-blue for hovered.
                strokeColor = '#9933FF';
            } else if (vertex.boundaryId !== '') {
                fillColor = 'white'; // White for boundary vertices.
                strokeColor = VERTEX_BORDER_COLOR;
            }
            
            // Draw vertex circle.
            this.ctx.beginPath();
            this.ctx.arc(x, y, VERTEX_RADIUS, 0, 2 * Math.PI);
            this.ctx.fillStyle = fillColor;
            this.ctx.fill();
            this.ctx.strokeStyle = strokeColor;
            this.ctx.lineWidth = 2;
            this.ctx.stroke();
        });
        
        // Draw preview vertex if hovering (only if not over a vertex).
        if (this.hoverPosition && this.hoveredVertexIndex === null) {
            const { x, y } = this.toScreen(this.hoverPosition.x, this.hoverPosition.y);
            
            // Draw preview vertex circle with semi-transparent fill.
            this.ctx.beginPath();
            this.ctx.arc(x, y, VERTEX_RADIUS, 0, 2 * Math.PI);
            this.ctx.fillStyle = PREVIEW_COLOR;
            this.ctx.fill();
            this.ctx.strokeStyle = PREVIEW_BORDER_COLOR;
            this.ctx.lineWidth = 2;
            this.ctx.stroke();
        }
        
        // Draw line from selected vertex to cursor if in edge creation mode.
        if (this.selectedVertexIndex !== null && this.hoverPosition) {
            const selectedVertex = this.graphTemplate.vertices[this.selectedVertexIndex];
            const from = this.toScreen(selectedVertex.position.x, selectedVertex.position.y);
            const to = this.toScreen(this.hoverPosition.x, this.hoverPosition.y);
            this.ctx.beginPath();
            this.ctx.moveTo(from.x, from.y);
            this.ctx.lineTo(to.x, to.y);
            this.ctx.strokeStyle = 'rgba(187, 102, 255, 0.5)'; // Purple-blue to match vertex colors
            this.ctx.lineWidth = 2;
            this.ctx.setLineDash([4, 4]);
            this.ctx.stroke();
            this.ctx.setLineDash([]);
        }
    }
    
    // Recompute derived template state and redraw the canvas.
    refreshTemplate() {
        updateTemplate(this.graphTemplate);
        this.drawVertices();
    }
    
    // Convert canvas coordinates to normalized coordinates (0-1 range) for comparison.
    canvasToNormalized(x, y) {
        return {
            x: this.canvas.width > 0 ? x / this.canvas.width : 0,
            y: this.canvas.height > 0 ? y / this.canvas.height : 0
        };
    }
    
    // Convert normalized coordinates back to canvas coordinates.
    normalizedToCanvas(normX, normY, targetCanvas) {
        return {
            x: normX * targetCanvas.width,
            y: normY * targetCanvas.height
        };
    }
    
    // Find vertex at position in other template (within tolerance).
    // Uses normalized coordinates to handle different canvas sizes.
    findVertexAtPosition(otherTemplate, x, y, tolerance = 0.01) {
        // Convert to normalized coordinates.
        const norm = this.canvasToNormalized(x, y);
        
        for (let i = 0; i < otherTemplate.graphTemplate.vertices.length; i++) {
            const v = otherTemplate.graphTemplate.vertices[i];
            // Convert other template's vertex position to normalized coordinates.
            const vNorm = otherTemplate.canvasToNormalized(v.position.x, v.position.y);
            
            const dx = Math.abs(vNorm.x - norm.x);
            const dy = Math.abs(vNorm.y - norm.y);
            if (dx <= tolerance && dy <= tolerance) {
                return i;
            }
        }
        return null;
    }
    
    // Add boundary vertex in other template when boundaryId changes from empty to non-empty.
    addBoundaryVertexInOtherTemplate(position, boundaryId) {
        if (!window.editor1 || !window.editor2) return;
        
        const otherEditor = this.templateId === 1 ? window.editor2 : window.editor1;
        
        // Convert position to normalized coordinates, then to other canvas coordinates.
        const norm = this.canvasToNormalized(position.x, position.y);
        const otherPosition = this.normalizedToCanvas(norm.x, norm.y, otherEditor.canvas);
        
        // Add new boundary vertex at same relative position in other canvas with the same boundaryId.
        const newVertex = {
            connections: [],
            boundaryId: boundaryId,
            position: { x: otherPosition.x, y: otherPosition.y }
        };
        otherEditor.graphTemplate.vertices.push(newVertex);
        otherEditor.refreshTemplate();
    }
    
    // Remove boundary vertex from other template when boundaryId changes from non-empty to empty.
    removeBoundaryVertexInOtherTemplate(boundaryId) {
        if (!window.editor1 || !window.editor2) return;
        
        const otherEditor = this.templateId === 1 ? window.editor2 : window.editor1;
        
        // Find vertex with the same boundaryId in other template.
        const vertexIndex = otherEditor.graphTemplate.vertices.findIndex(v => v.boundaryId === boundaryId);
        
        if (vertexIndex !== -1) {
            const vertex = otherEditor.graphTemplate.vertices[vertexIndex];
            const edgesToRemove = [...vertex.connections].sort((a, b) => b - a);

            otherEditor.graphTemplate.vertices.splice(vertexIndex, 1);
            if (otherEditor.selectedVertexIndex === vertexIndex) {
                otherEditor.selectedVertexIndex = null;
            } else if (otherEditor.selectedVertexIndex > vertexIndex) {
                otherEditor.selectedVertexIndex--;
            }

            // Edges to a boundary placeholder only exist on this template; delete them entirely.
            for (const edgeIndex of edgesToRemove) {
                otherEditor.removeEdgeFromGraph(edgeIndex);
            }

            otherEditor.refreshTemplate();
        }
    }
    
    // Handle file import data.
    handleFileImportData(imported) {
        const { vertices, edges } = importGraphWithSplices(imported);
        this.graphTemplate.vertices = vertices;
        this.graphTemplate.edges = edges;
        this.selectedVertexIndex = null;
        this.hoveredVertexIndex = null;
        this.refreshTemplate();
    }
    
}

// Global editor instances.
let editor1, editor2;

// Library state. Each entry: { comment, graphs: [graphTemplate, ...] }.
// activeIdx === -1 means the canvas pair is a "scratch" not bound to any
// library entry. cleanState is a JSON.stringify of the canvas pair at the
// moment it was last saved/loaded, used to detect unsaved edits on nav.
window.library = [];
window.activeIdx = -1;
let cleanState = null;

function getPairSerialized() {
    return JSON.stringify({
        e1: window.editor1.graphTemplate,
        e2: window.editor2.graphTemplate,
    });
}

function markClean() {
    cleanState = getPairSerialized();
}

function hasUnsavedEdits() {
    return cleanState !== null && cleanState !== getPairSerialized();
}

function clearBothCanvases() {
    editor1.graphTemplate.vertices = [];
    editor1.graphTemplate.edges = [];
    editor1.selectedVertexIndex = null;
    editor1.hoveredVertexIndex = null;
    editor2.graphTemplate.vertices = [];
    editor2.graphTemplate.edges = [];
    editor2.selectedVertexIndex = null;
    editor2.hoveredVertexIndex = null;
    editor1.refreshTemplate();
    editor2.refreshTemplate();
}

// Distinguish the three import shapes the editor accepts:
//   library      : array of { comment?, graphs: [...] }
//   per-pair     : array of length 2 of { vertices, edges }
//   single       : object with { vertices, edges }
function detectImportShape(data) {
    if (Array.isArray(data)) {
        if (data.length > 0 && data[0] && Array.isArray(data[0].graphs)) return 'library';
        if (data.length >= 1 && data[0] && Array.isArray(data[0].vertices)) return 'pair';
        return 'unknown';
    }
    if (data && Array.isArray(data.vertices)) return 'single';
    return 'unknown';
}

function loadLibraryEntry(idx) {
    const entry = window.library[idx];
    if (!entry) return;
    const g0 = entry.graphs[0];
    const g1 = entry.graphs[1];
    if (g0) {
        editor1.handleFileImportData(g0);
    } else {
        editor1.graphTemplate.vertices = [];
        editor1.graphTemplate.edges = [];
        editor1.selectedVertexIndex = null;
        editor1.refreshTemplate();
    }
    if (g1) {
        editor2.handleFileImportData(g1);
    } else {
        editor2.graphTemplate.vertices = [];
        editor2.graphTemplate.edges = [];
        editor2.selectedVertexIndex = null;
        editor2.refreshTemplate();
    }
    window.activeIdx = idx;
    markClean();
    updateLibraryUI();
}

function updateLibraryUI() {
    const status = document.getElementById('libStatus');
    const prev = document.getElementById('libPrevBtn');
    const next = document.getElementById('libNextBtn');
    const del = document.getElementById('libDeleteBtn');
    const comment = document.getElementById('libComment');
    if (!status) return;

    const n = window.library.length;
    const idx = window.activeIdx;
    if (n === 0) {
        status.textContent = 'Empty';
    } else if (idx < 0) {
        status.textContent = `(unsaved) / ${n}`;
    } else {
        status.textContent = `Entry ${idx + 1} / ${n}`;
    }

    prev.disabled = n === 0;
    next.disabled = n === 0;
    del.disabled = idx < 0 || idx >= n;
    comment.disabled = idx < 0 || idx >= n;
    if (idx >= 0 && idx < n) {
        const c = window.library[idx].comment || '';
        if (document.activeElement !== comment) comment.value = c;
    } else {
        comment.value = '';
    }
}

function snapshotCurrentPair() {
    return {
        comment: '',
        graphs: [
            cloneGraphData(editor1.graphTemplate),
            cloneGraphData(editor2.graphTemplate),
        ],
    };
}

function serializeLibraryEntry(entry) {
    return {
        comment: typeof entry.comment === 'string' ? entry.comment : '',
        graphs: Array.isArray(entry.graphs)
            ? entry.graphs.map(g => cloneGraphData(g))
            : [],
    };
}

function serializeLibraryEntryForExport(entry) {
    return {
        comment: entry.comment ?? '',
        graphs: Array.isArray(entry.graphs)
            ? entry.graphs.map(g => exportGraphWithSplices(g))
            : [],
    };
}

function confirmDiscardUnsaved() {
    if (!hasUnsavedEdits()) return true;
    return window.confirm('You have unsaved edits to the current entry. Discard them?');
}

// Initialize both template editors on page load.
document.addEventListener('DOMContentLoaded', () => {
    editor1 = new TemplateEditor(1);
    editor1.init();
    
    editor2 = new TemplateEditor(2);
    editor2.init();
    
    // Store references globally so editors can access each other for syncing.
    window.editor1 = editor1;
    window.editor2 = editor2;
    
    // Set up global action handlers.
    const clearAllBtn = document.getElementById('clearAllBtn');
    const exportAllBtn = document.getElementById('exportAllBtn');
    const exportPairBtn = document.getElementById('exportPairBtn');
    const importAllBtn = document.getElementById('importAllBtn');
    const importAllFile = document.getElementById('importAllFile');
    const undoBtn = document.getElementById('undoBtn');
    const redoBtn = document.getElementById('redoBtn');
    const libPrevBtn = document.getElementById('libPrevBtn');
    const libNextBtn = document.getElementById('libNextBtn');
    const libSaveBtn = document.getElementById('libSaveBtn');
    const libNewBtn = document.getElementById('libNewBtn');
    const libDeleteBtn = document.getElementById('libDeleteBtn');
    const libComment = document.getElementById('libComment');

    clearAllBtn.addEventListener('click', () => {
        History.commit();
        clearBothCanvases();
    });

    // Export the whole library as a JSON array. If the user has an unsaved
    // scratch pair (activeIdx === -1) and the library is empty, fall back to
    // exporting just that pair as a one-entry library so the file is never
    // empty when something useful is on the canvas.
    exportAllBtn.addEventListener('click', () => {
        let out;
        if (window.library.length === 0) {
            out = [{
                comment: '',
                graphs: [
                    exportGraphWithSplices(editor1.graphTemplate),
                    exportGraphWithSplices(editor2.graphTemplate),
                ],
            }];
        } else {
            out = window.library.map(serializeLibraryEntryForExport);
        }
        const json = JSON.stringify(out, null, 2);
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'graph_templates.json';
        a.click();
        URL.revokeObjectURL(url);
    });

    // Per-pair export.
    exportPairBtn.addEventListener('click', () => {
        const combined = [
            exportGraphWithSplices(editor1.graphTemplate),
            exportGraphWithSplices(editor2.graphTemplate),
        ];
        const json = JSON.stringify(combined, null, 2);
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'graph_template_pair.json';
        a.click();
        URL.revokeObjectURL(url);
    });

    importAllBtn.addEventListener('click', () => {
        importAllFile.click();
    });

    importAllFile.addEventListener('change', (event) => {
        const files = Array.from(event.target.files);
        if (files.length === 0) return;

        History.commit();

        // Single-file path: distinguish library / per-pair / single shapes.
        if (files.length === 1) {
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const imported = JSON.parse(e.target.result);
                    const shape = detectImportShape(imported);
                    if (shape === 'library') {
                        window.library = imported.map(entry => serializeLibraryEntry({
                            comment: typeof entry.comment === 'string' ? entry.comment : '',
                            graphs: Array.isArray(entry.graphs) ? entry.graphs : [],
                        }));
                        if (window.library.length > 0) {
                            loadLibraryEntry(0);
                        } else {
                            clearBothCanvases();
                            window.activeIdx = -1;
                            markClean();
                            updateLibraryUI();
                        }
                    } else if (shape === 'pair') {
                        window.library = [serializeLibraryEntry({
                            comment: '',
                            graphs: imported.slice(0, 2),
                        })];
                        loadLibraryEntry(0);
                    } else if (shape === 'single') {
                        window.library = [serializeLibraryEntry({
                            comment: '',
                            graphs: [imported],
                        })];
                        loadLibraryEntry(0);
                    } else {
                        console.error('Unrecognized import shape');
                    }
                } catch (error) {
                    console.error('Error importing file:', error.message);
                }
            };
            reader.readAsText(files[0]);
        } else {
            // Multiple files: first file into editor1, second into editor2.
            const reader1 = new FileReader();
            reader1.onload = (e) => {
                try {
                    const imported = JSON.parse(e.target.result);
                    const template = Array.isArray(imported) ? imported[0] : imported;
                    editor1.handleFileImportData(template);
                } catch (error) {
                    console.error('Error importing file 1:', error.message);
                }

                if (files.length >= 2) {
                    const reader2 = new FileReader();
                    reader2.onload = (e2) => {
                        try {
                            const imported2 = JSON.parse(e2.target.result);
                            const template2 = Array.isArray(imported2) ? imported2[0] : imported2;
                            editor2.handleFileImportData(template2);
                        } catch (error) {
                            console.error('Error importing file 2:', error.message);
                        }
                        window.activeIdx = -1;
                        markClean();
                        updateLibraryUI();
                    };
                    reader2.readAsText(files[1]);
                } else {
                    window.activeIdx = -1;
                    markClean();
                    updateLibraryUI();
                }
            };
            reader1.readAsText(files[0]);
        }

        importAllFile.value = '';
    });

    // Undo / redo buttons.
    undoBtn.addEventListener('click', () => History.undo());
    redoBtn.addEventListener('click', () => History.redo());

    // Library navigation. Non-destructive; prompts on unsaved edits.
    libPrevBtn.addEventListener('click', () => {
        if (window.library.length === 0) return;
        if (!confirmDiscardUnsaved()) return;
        const n = window.library.length;
        const cur = window.activeIdx < 0 ? 0 : window.activeIdx;
        const next = (cur - 1 + n) % n;
        loadLibraryEntry(next);
    });

    libNextBtn.addEventListener('click', () => {
        if (window.library.length === 0) return;
        if (!confirmDiscardUnsaved()) return;
        const n = window.library.length;
        const cur = window.activeIdx < 0 ? -1 : window.activeIdx;
        const next = (cur + 1) % n;
        loadLibraryEntry(next);
    });

    // Persist the canvas pair to the library. If we're on an active entry,
    // overwrite it; if we're in scratch state (activeIdx === -1), push a
    // new entry. Canvas stays so the user can keep editing; the "New entry"
    // button is the explicit way to clear and start fresh.
    libSaveBtn.addEventListener('click', () => {
        History.commit();
        const entry = snapshotCurrentPair();
        if (window.activeIdx < 0) {
            entry.comment = (libComment.value || '').trim();
            window.library.push(entry);
            window.activeIdx = window.library.length - 1;
        } else {
            const keepComment = window.library[window.activeIdx].comment || '';
            entry.comment = (libComment.value || keepComment).trim();
            window.library[window.activeIdx] = entry;
        }
        markClean();
        updateLibraryUI();
    });

    // Clear the canvas to start a new entry. Prompts if there are unsaved
    // edits on the current entry so the user doesn't lose work by accident.
    libNewBtn.addEventListener('click', () => {
        if (!confirmDiscardUnsaved()) return;
        History.commit();
        clearBothCanvases();
        window.activeIdx = -1;
        libComment.value = '';
        markClean();
        updateLibraryUI();
    });

    libDeleteBtn.addEventListener('click', () => {
        if (window.activeIdx < 0 || window.activeIdx >= window.library.length) return;
        History.commit();
        window.library.splice(window.activeIdx, 1);
        if (window.library.length === 0) {
            window.activeIdx = -1;
            clearBothCanvases();
            markClean();
        } else {
            const next = Math.min(window.activeIdx, window.library.length - 1);
            loadLibraryEntry(next);
        }
        updateLibraryUI();
    });

    libComment.addEventListener('input', () => {
        if (window.activeIdx < 0 || window.activeIdx >= window.library.length) return;
        window.library[window.activeIdx].comment = libComment.value;
    });

    // Keyboard shortcuts for undo/redo. Skip when typing in an input.
    window.addEventListener('keydown', (event) => {
        if (isTypingInTextInput(event.target)) return;
        if (!event.ctrlKey && !event.metaKey) return;
        const key = event.key.toLowerCase();
        if (key === 'z' && !event.shiftKey) {
            event.preventDefault();
            History.undo();
        } else if ((key === 'z' && event.shiftKey) || key === 'y') {
            event.preventDefault();
            History.redo();
        }
    });

    markClean();
    updateLibraryUI();
    updateHistoryButtons();
});
