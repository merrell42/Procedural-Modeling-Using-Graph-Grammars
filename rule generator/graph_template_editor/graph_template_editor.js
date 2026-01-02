// Vertex rendering settings (shared constants).
const VERTEX_RADIUS = 12;
const VERTEX_COLOR = '#2196F3';
const VERTEX_BORDER_COLOR = '#1976D2';
const PREVIEW_COLOR = 'rgba(33, 150, 243, 0.3)';
const PREVIEW_BORDER_COLOR = 'rgba(25, 118, 210, 0.4)';

// TemplateEditor class to manage a single graph template.
class TemplateEditor {
    constructor(templateId) {
        this.templateId = templateId;
        
        // Graph template data structure.
        this.graphTemplate = {
            vertices: [],
            numEdges: 0
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
    
    // Initialize canvas.
    init() {
        // Make canvas responsive to fit container.
        this.resizeCanvas();
        window.addEventListener('resize', () => this.resizeCanvas());
        
        this.canvas.addEventListener('click', this.handleCanvasClick);
        this.canvas.addEventListener('dblclick', this.handleCanvasDoubleClick);
        this.canvas.addEventListener('mousemove', this.handleCanvasMouseMove);
        this.canvas.addEventListener('mouseleave', this.handleCanvasMouseLeave);
        
        
        this.updateDisplay();
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
        this.updateDisplay();
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
    
    // Update onBoundary property for a specific vertex based on connection count.
    updateOnBoundaryForVertex(vertex) {
        const wasBoundary = vertex.onBoundary;
        vertex.onBoundary = vertex.connections.length === 1;
        
        // If onBoundary changed, add or remove vertex in other template.
        if (!wasBoundary && vertex.onBoundary) {
            // Changed from false to true - add vertex in other template.
            this.addBoundaryVertexInOtherTemplate(vertex.position);
        } else if (wasBoundary && !vertex.onBoundary) {
            // Changed from true to false - delete vertex in other template.
            this.removeBoundaryVertexInOtherTemplate(vertex.position);
        }
    }
    
    // Insert edge in counter-clockwise order around a vertex.
    insertEdgeInOrder(vertex, edgeIndex, newAngle) {
        // Normalize angle to [0, 2π] range.
        const normalizedAngle = newAngle < 0 ? newAngle + 2 * Math.PI : newAngle;
        
        // Find the insertion position by comparing angles.
        let insertIndex = vertex.connections.length;
        
        for (let i = 0; i < vertex.connections.length; i++) {
            const existingEdgeIndex = vertex.connections[i];
            // Find the other vertex connected by this edge.
            const otherVertex = this.graphTemplate.vertices.find((v, idx) => 
                v !== vertex && v.connections.includes(existingEdgeIndex)
            );
            
            if (otherVertex) {
                const existingAngle = Math.atan2(
                    otherVertex.position.y - vertex.position.y,
                    otherVertex.position.x - vertex.position.x
                );
                const normalizedExistingAngle = existingAngle < 0 ? existingAngle + 2 * Math.PI : existingAngle;
                
                // Insert before this edge if the new angle is smaller (counter-clockwise order).
                if (normalizedAngle < normalizedExistingAngle) {
                    insertIndex = i;
                    break;
                }
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
        
        // Calculate position in canvas coordinate space.
        const x = clampedX * scaleX;
        const y = clampedY * scaleY;
        return { x, y };
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
    
    // Handle canvas double-click to toggle onBoundary.
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
            const wasBoundary = vertex.onBoundary;
            const position = { x: vertex.position.x, y: vertex.position.y };
            
            // Toggle onBoundary.
            if (vertex.onBoundary) {
                // Set to false.
                vertex.onBoundary = false;
                this.updateDisplay();
                
                // Remove boundary vertex at same position in other template.
                this.removeBoundaryVertexInOtherTemplate(position);
            } else if (vertex.connections.length === 1) {
                // Only allow setting to true if vertex has exactly one edge.
                vertex.onBoundary = true;
                this.updateDisplay();
                
                // Add boundary vertex at same position in other template.
                this.addBoundaryVertexInOtherTemplate(position);
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
        // Check if clicking on a vertex.
        const vertexIndex = this.findVertexAt(x, y);
        
        if (vertexIndex !== null) {
            // Clicked on a vertex - handle edge creation.
            if (this.selectedVertexIndex === null) {
                // First vertex selected.
                this.selectedVertexIndex = vertexIndex;
            } else if (this.selectedVertexIndex === vertexIndex) {
                // Clicked the same vertex - deselect.
                this.selectedVertexIndex = null;
            } else {
                // Second vertex selected - create edge.
                const v1 = this.graphTemplate.vertices[this.selectedVertexIndex];
                const v2 = this.graphTemplate.vertices[vertexIndex];
                
                // Check if edge already exists (vertices share a common edge index).
                const sharedEdge = v1.connections.find(edgeIndex => v2.connections.includes(edgeIndex));
                
                if (sharedEdge === undefined) {
                    // Add edge indices (using current numEdges as the edge index).
                    const edgeIndex = this.graphTemplate.numEdges;
                    
                    // Calculate angle from v1 to v2 for counter-clockwise ordering.
                    const angle1 = Math.atan2(v2.position.y - v1.position.y, v2.position.x - v1.position.x);
                    this.insertEdgeInOrder(v1, edgeIndex, angle1);
                    
                    // Calculate angle from v2 to v1 for counter-clockwise ordering.
                    const angle2 = Math.atan2(v1.position.y - v2.position.y, v1.position.x - v2.position.x);
                    this.insertEdgeInOrder(v2, edgeIndex, angle2);
                    
                    this.graphTemplate.numEdges++;
                    
                    // Update onBoundary for the two vertices involved in the edge.
                    this.updateOnBoundaryForVertex(v1);
                    this.updateOnBoundaryForVertex(v2);
                    
                } else {
                }
                
                this.selectedVertexIndex = null;
            }
            this.updateDisplay();
        } else {
            // Clicked on empty space.
            if (this.selectedVertexIndex === null) {
                // Not in edge creation mode - add new vertex.
                const vertex = {
                    connections: [],
                    onBoundary: false,
                    position: { x: x, y: y }
                };
                
                this.graphTemplate.vertices.push(vertex);
                this.updateDisplay();
            } else {
                // In edge creation mode - add vertex and connect it to selected vertex.
                const newVertex = {
                    connections: [],
                    onBoundary: false,
                    position: { x: x, y: y }
                };
                const newVertexIndex = this.graphTemplate.vertices.length;
                this.graphTemplate.vertices.push(newVertex);
                
                // Create edge between selected vertex and new vertex.
                const v1 = this.graphTemplate.vertices[this.selectedVertexIndex];
                const edgeIndex = this.graphTemplate.numEdges;
                
                // Calculate angle from v1 to new vertex for counter-clockwise ordering.
                const angle1 = Math.atan2(newVertex.position.y - v1.position.y, newVertex.position.x - v1.position.x);
                this.insertEdgeInOrder(v1, edgeIndex, angle1);
                
                // Calculate angle from new vertex to v1 for counter-clockwise ordering.
                const angle2 = Math.atan2(v1.position.y - newVertex.position.y, v1.position.x - newVertex.position.x);
                this.insertEdgeInOrder(newVertex, edgeIndex, angle2);
                
                this.graphTemplate.numEdges++;
                
                // Update onBoundary for the two vertices involved in the edge.
                this.updateOnBoundaryForVertex(v1);
                this.updateOnBoundaryForVertex(newVertex);
                
                const selectedIndex = this.selectedVertexIndex;
                this.selectedVertexIndex = null;
                this.updateDisplay();
            }
        }
    }
    
    // Draw all vertices on canvas.
    drawVertices() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Draw edges (only draw each edge once).
        const drawnEdges = new Set();
        this.graphTemplate.vertices.forEach((vertex, index) => {
            vertex.connections.forEach(edgeIndex => {
                if (!drawnEdges.has(edgeIndex)) {
                    // Find the other vertex connected by this edge.
                    const otherVertex = this.graphTemplate.vertices.find((v, i) => 
                        i !== index && v.connections.includes(edgeIndex)
                    );
                    if (otherVertex) {
                        this.ctx.beginPath();
                        this.ctx.moveTo(vertex.position.x, vertex.position.y);
                        this.ctx.lineTo(otherVertex.position.x, otherVertex.position.y);
                        this.ctx.strokeStyle = '#0066FF';
                        this.ctx.lineWidth = 1;
                        this.ctx.stroke();
                        drawnEdges.add(edgeIndex);
                    }
                }
            });
        });
        
        // Draw existing vertices.
        this.graphTemplate.vertices.forEach((vertex, index) => {
            const x = vertex.position.x;
            const y = vertex.position.y;
            
            // Determine vertex color based on state.
            let fillColor = VERTEX_COLOR;
            let strokeColor = VERTEX_BORDER_COLOR;
            
            if (index === this.selectedVertexIndex) {
                    fillColor = '#BB66FF'; // Brighter purple-blue for selected.
                strokeColor = '#9933FF';
            } else if (index === this.hoveredVertexIndex) {
                fillColor = '#BB66FF'; // Brighter purple-blue for hovered.
                strokeColor = '#9933FF';
            } else if (vertex.onBoundary) {
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
            const x = this.hoverPosition.x;
            const y = this.hoverPosition.y;
            
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
            this.ctx.beginPath();
            this.ctx.moveTo(selectedVertex.position.x, selectedVertex.position.y);
            this.ctx.lineTo(this.hoverPosition.x, this.hoverPosition.y);
            this.ctx.strokeStyle = 'rgba(187, 102, 255, 0.5)'; // Purple-blue to match vertex colors
            this.ctx.lineWidth = 2;
            this.ctx.setLineDash([4, 4]);
            this.ctx.stroke();
            this.ctx.setLineDash([]);
        }
    }
    
    // Update all displays.
    updateDisplay() {
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
    
    // Add boundary vertex in other template when onBoundary changes from false to true.
    addBoundaryVertexInOtherTemplate(position) {
        if (!window.editor1 || !window.editor2) return;
        
        const otherEditor = this.templateId === 1 ? window.editor2 : window.editor1;
        
        // Convert position to normalized coordinates, then to other canvas coordinates.
        const norm = this.canvasToNormalized(position.x, position.y);
        const otherPosition = this.normalizedToCanvas(norm.x, norm.y, otherEditor.canvas);
        
        // Add new boundary vertex at same relative position in other canvas.
        const newVertex = {
            connections: [],
            onBoundary: true,
            position: { x: otherPosition.x, y: otherPosition.y }
        };
        otherEditor.graphTemplate.vertices.push(newVertex);
        otherEditor.updateDisplay();
    }
    
    // Remove boundary vertex from other template when onBoundary changes from true to false.
    removeBoundaryVertexInOtherTemplate(position) {
        if (!window.editor1 || !window.editor2) return;
        
        const otherEditor = this.templateId === 1 ? window.editor2 : window.editor1;
        
        // Find vertex at same relative position in other template.
        const vertexIndex = this.findVertexAtPosition(otherEditor, position.x, position.y);
        
        if (vertexIndex !== null) {
            // Delete the vertex.
            otherEditor.graphTemplate.vertices.splice(vertexIndex, 1);
            if (otherEditor.selectedVertexIndex === vertexIndex) {
                otherEditor.selectedVertexIndex = null;
            } else if (otherEditor.selectedVertexIndex > vertexIndex) {
                otherEditor.selectedVertexIndex--;
            }
            otherEditor.updateDisplay();
        }
    }
    
    // Handle file import data.
    handleFileImportData(imported) {
        if (imported.vertices && Array.isArray(imported.vertices)) {
            // Validate and normalize vertex data.
            this.graphTemplate.vertices = imported.vertices.map((v, index) => {
                // Ensure all required fields exist.
                return {
                    connections: Array.isArray(v.connections) ? v.connections : [],
                    onBoundary: typeof v.onBoundary === 'boolean' ? v.onBoundary : false,
                    position: v.position && typeof v.position.x === 'number' && typeof v.position.y === 'number'
                        ? { x: v.position.x, y: v.position.y }
                        : { x: 100 + index * 50, y: 100 } // Default position if missing.
                };
            });
            this.graphTemplate.numEdges = typeof imported.numEdges === 'number' ? imported.numEdges : 0;
            this.selectedVertexIndex = null;
            this.hoveredVertexIndex = null;
            
            this.updateDisplay();
        } else {
            throw new Error('Invalid format: missing or invalid "vertices" array');
        }
    }
    
}

// Global editor instances.
let editor1, editor2;

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
    const importAllBtn = document.getElementById('importAllBtn');
    const importAllFile = document.getElementById('importAllFile');
    
    clearAllBtn.addEventListener('click', () => {
        editor1.graphTemplate.vertices = [];
        editor1.graphTemplate.numEdges = 0;
        editor1.selectedVertexIndex = null;
        editor1.hoveredVertexIndex = null;
        editor1.updateDisplay();
        
        editor2.graphTemplate.vertices = [];
        editor2.graphTemplate.numEdges = 0;
        editor2.selectedVertexIndex = null;
        editor2.hoveredVertexIndex = null;
        editor2.updateDisplay();
    });
    
    exportAllBtn.addEventListener('click', () => {
        // Export both templates as a combined JSON array.
        const combined = [editor1.graphTemplate, editor2.graphTemplate];
        const json = JSON.stringify(combined, null, 2);
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'graph_templates.json';
        a.click();
        URL.revokeObjectURL(url);
    });
    
    importAllBtn.addEventListener('click', () => {
        importAllFile.click();
    });
    
    importAllFile.addEventListener('change', (event) => {
        const files = Array.from(event.target.files);
        if (files.length === 0) return;
        
        // If single file, check if it's an array of templates.
        if (files.length === 1) {
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const imported = JSON.parse(e.target.result);
                    if (Array.isArray(imported) && imported.length >= 2) {
                        // Array of templates - import first two.
                        editor1.handleFileImportData(imported[0]);
                        editor2.handleFileImportData(imported[1]);
                    } else if (Array.isArray(imported) && imported.length === 1) {
                        // Array with one template - import to template 1.
                        editor1.handleFileImportData(imported[0]);
                    } else {
                        // Single template - import to template 1.
                        editor1.handleFileImportData(imported);
                    }
                } catch (error) {
                    console.error('Error importing file:', error.message);
                }
            };
            reader.readAsText(files[0]);
        } else {
            // Multiple files - import first to template 1, second to template 2.
            const reader1 = new FileReader();
            reader1.onload = (e) => {
                try {
                    const imported = JSON.parse(e.target.result);
                    const template = Array.isArray(imported) ? imported[0] : imported;
                    editor1.handleFileImportData(template);
                } catch (error) {
                    console.error('Error importing file 1:', error.message);
                }
                
                // Import second file if available.
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
                    };
                    reader2.readAsText(files[1]);
                }
            };
            reader1.readAsText(files[0]);
        }
        
        importAllFile.value = '';
    });
});
