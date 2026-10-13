// Decoration editing for the primitives editor.
//
// The file keeps decorations at doc.decorations.vertex, .edge, and .face. A face,
// edge, or vertex type points at one by setting "decoration" to its id. A
// vertex type can only point at a vertex decoration, an edge type at an edge
// decoration, and a face type at a face decoration. The parameters of each
// decoration type are described in cpp_version/decorations/README.md.
(function () {
    'use strict';

    var KINDS = ['vertex', 'edge', 'face'];
    var getDoc = function () { return null; };
    var rerender = function () {};

    // ----- Decoration types ---------------------------------------------
    // defaults: values a new decoration of this type starts with.
    // fields: what the panel shows, in order. Required references have no default.
    function composite(kind) {
        return [
            {
                type: 'union',
                label: 'Union',
                defaults: { childIds: [] },
                fields: [{ control: 'refList', key: 'childIds', refKind: kind, label: 'Children' }],
            },
            {
                type: 'pick random',
                label: 'Pick random',
                defaults: { childIds: [] },
                fields: [{ control: 'refList', key: 'childIds', refKind: kind, label: 'Children', weights: true }],
            },
        ];
    }

    var TYPES = {
        vertex: [
            {
                type: 'place object',
                label: 'Place object',
                defaults: { object: '' },
                fields: [{ control: 'text', key: 'object', label: 'Object (asset id)' }],
            },
            {
                type: 'rotate',
                label: 'Rotate',
                defaults: { minAngle: 0, maxAngle: 360 },
                fields: [
                    { control: 'number', key: 'minAngle', label: 'Min angle (degrees)' },
                    { control: 'number', key: 'maxAngle', label: 'Max angle (degrees)' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child' },
                    { control: 'vec', key: 'axis', label: 'Set axis', optional: true, initial: [0, 0, 1], names: ['x', 'y', 'z'] },
                ],
            },
            {
                type: 'scale',
                label: 'Scale',
                defaults: { min: 1, max: 1 },
                fields: [
                    { control: 'scale' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child' },
                ],
            },
        ].concat(composite('vertex')),
        edge: [
            {
                type: 'space evenly',
                label: 'Space evenly',
                defaults: { spacing: 1 },
                fields: [
                    { control: 'number', key: 'spacing', label: 'Spacing' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child (vertex decoration)' },
                ],
            },
            {
                type: 'space randomly',
                label: 'Space randomly',
                defaults: { spacing: 1 },
                fields: [
                    { control: 'number', key: 'spacing', label: 'Spacing' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child (vertex decoration)' },
                ],
            },
            {
                type: 'extrude',
                label: 'Extrude',
                defaults: { profile: [[-0.1, -0.1], [0.1, -0.1], [0.1, 0.1], [-0.1, 0.1]] },
                fields: [
                    { control: 'profile' },
                    { control: 'vec', key: 'color', label: 'Set color', optional: true, initial: [1, 1, 1], names: ['r', 'g', 'b'], picker: true },
                    { control: 'number', key: 'scale', label: 'Scale', optional: true, placeholder: '1' },
                ],
            },
        ].concat(composite('edge')),
        face: [
            {
                type: 'scatter',
                label: 'Scatter',
                defaults: { density: 1 },
                fields: [
                    { control: 'number', key: 'density', label: 'Density (points per unit of area)' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child (vertex decoration)' },
                ],
            },
            {
                type: 'grid',
                label: 'Grid',
                defaults: { spacing: 1 },
                fields: [
                    { control: 'number', key: 'spacing', label: 'Spacing' },
                    { control: 'ref', key: 'child', refKind: 'vertex', label: 'Child (vertex decoration)' },
                ],
            },
            {
                type: 'slice',
                label: 'Slice',
                defaults: { direction: [1, 0, 0], spacing: 1 },
                fields: [
                    { control: 'vec', key: 'direction', label: 'Direction', names: ['x', 'y', 'z'] },
                    { control: 'number', key: 'spacing', label: 'Spacing' },
                    { control: 'ref', key: 'edge', refKind: 'edge', label: 'Edge decoration', optional: true },
                    { control: 'ref', key: 'startVertex', refKind: 'vertex', label: 'Start vertex decoration', optional: true },
                    { control: 'ref', key: 'endVertex', refKind: 'vertex', label: 'End vertex decoration', optional: true },
                ],
                warning: function (d) {
                    if (d.edge || d.startVertex || d.endVertex) return '';
                    return 'Choose an edge, start vertex, or end vertex decoration. With none of them the slice is not created.';
                },
            },
        ].concat(composite('face')),
    };

    function specFor(kind, typeName) {
        var specs = TYPES[kind];
        for (var i = 0; i < specs.length; i++) {
            if (specs[i].type === typeName) return specs[i];
        }
        return null;
    }

    function fieldKeys(field) {
        if (field.control === 'scale') return ['min', 'max'];
        if (field.control === 'profile') return ['svg', 'profile'];
        if (field.control === 'refList' && field.weights) return [field.key, 'weight'];
        return [field.key];
    }

    function clone(value) {
        return value === undefined ? undefined : JSON.parse(JSON.stringify(value));
    }

    // ----- Decorations in the document ----------------------------------
    function list(kind) {
        var doc = getDoc();
        var all = doc && doc.decorations;
        return all && Array.isArray(all[kind]) ? all[kind] : [];
    }

    function ensureList(kind) {
        var doc = getDoc();
        if (!doc.decorations || typeof doc.decorations !== 'object') doc.decorations = {};
        KINDS.forEach(function (k) {
            if (!Array.isArray(doc.decorations[k])) doc.decorations[k] = [];
        });
        return doc.decorations[kind];
    }

    function find(kind, id) {
        var items = list(kind);
        for (var i = 0; i < items.length; i++) {
            if (items[i].id === id) return items[i];
        }
        return null;
    }

    function uniqueId(kind, base) {
        var n = 1;
        while (find(kind, base + ' ' + n)) n++;
        return base + ' ' + n;
    }

    function applySpec(deco, spec, old) {
        var keep = old || {};
        spec.fields.forEach(function (field) {
            fieldKeys(field).forEach(function (key) {
                if (keep[key] !== undefined) deco[key] = keep[key];
                else if (spec.defaults[key] !== undefined) deco[key] = clone(spec.defaults[key]);
            });
        });
    }

    // Returns the id of the new decoration.
    function createDecoration(kind) {
        var spec = TYPES[kind][0];
        var deco = { id: uniqueId(kind, spec.type), type: spec.type };
        applySpec(deco, spec, null);
        ensureList(kind).push(deco);
        return deco.id;
    }

    function changeType(kind, deco, typeName) {
        var spec = specFor(kind, typeName);
        if (!spec) return;
        var old = clone(deco);
        Object.keys(deco).forEach(function (key) { delete deco[key]; });
        deco.id = old.id;
        deco.type = typeName;
        applySpec(deco, spec, old);
    }

    // ----- References between decorations -------------------------------
    // A slot is one place that holds the id of a decoration.
    function slotsOf(ownerKind, deco) {
        var slots = [];
        [['child', 'vertex'], ['edge', 'edge'], ['startVertex', 'vertex'], ['endVertex', 'vertex']].forEach(function (entry) {
            if (typeof deco[entry[0]] === 'string') {
                slots.push({ kind: entry[1], owner: deco, key: entry[0], index: -1, id: deco[entry[0]] });
            }
        });
        if (Array.isArray(deco.childIds)) {
            deco.childIds.forEach(function (id, index) {
                slots.push({ kind: ownerKind, owner: deco, key: 'childIds', index: index, id: id });
            });
        }
        return slots;
    }

    function allSlots() {
        var doc = getDoc();
        var types = doc.types || {};
        var slots = [];
        [['face', 'faceTypes'], ['edge', 'edgeTypes'], ['vertex', 'vertexTypes']].forEach(function (entry) {
            (types[entry[1]] || []).forEach(function (type) {
                if (typeof type.decoration === 'string') {
                    slots.push({ kind: entry[0], owner: type, key: 'decoration', index: -1, id: type.decoration });
                }
            });
        });
        KINDS.forEach(function (kind) {
            list(kind).forEach(function (deco) { slots = slots.concat(slotsOf(kind, deco)); });
        });
        return slots;
    }

    function reaches(kind, fromId, targetId, seen) {
        if (seen[fromId]) return false;
        seen[fromId] = true;
        var deco = find(kind, fromId);
        if (!deco) return false;
        return slotsOf(kind, deco).some(function (slot) {
            if (slot.kind !== kind) return false;
            return slot.id === targetId || reaches(kind, slot.id, targetId, seen);
        });
    }

    // Choosing candidateId under ownerId must not make a decoration contain itself.
    function wouldCycle(kind, ownerId, candidateId) {
        return candidateId === ownerId || reaches(kind, candidateId, ownerId, {});
    }

    function renameDecoration(kind, deco, newId) {
        var oldId = deco.id;
        allSlots().forEach(function (slot) {
            if (slot.kind !== kind || slot.id !== oldId) return;
            if (slot.index < 0) slot.owner[slot.key] = newId;
            else slot.owner.childIds[slot.index] = newId;
        });
        deco.id = newId;
        onRenamed(kind, oldId, newId);
    }

    function deleteDecoration(kind, deco) {
        var slots = allSlots();
        for (var i = slots.length - 1; i >= 0; i--) {
            var slot = slots[i];
            if (slot.kind !== kind || slot.id !== deco.id) continue;
            if (slot.index < 0) {
                delete slot.owner[slot.key];
            } else {
                slot.owner.childIds.splice(slot.index, 1);
                if (Array.isArray(slot.owner.weight)) slot.owner.weight.splice(slot.index, 1);
            }
        }
        var items = ensureList(kind);
        items.splice(items.indexOf(deco), 1);
    }

    // ----- Small controls -----------------------------------------------
    function el(tag, className, text) {
        var node = document.createElement(tag);
        if (className) node.className = className;
        if (text != null) node.textContent = text;
        return node;
    }

    function formatNum(n) {
        if (!Number.isFinite(n)) return '';
        return String(Math.round(n * 1e6) / 1e6);
    }

    function addNote(parent, text, className) {
        parent.appendChild(el('p', className || 'note', text));
    }

    function addCheck(parent, label, checked, onChange) {
        var checks = el('div', 'checks');
        var wrap = el('label', null);
        var input = document.createElement('input');
        input.type = 'checkbox';
        input.checked = checked;
        input.addEventListener('change', function () { onChange(input.checked); });
        wrap.appendChild(input);
        wrap.appendChild(document.createTextNode(label));
        checks.appendChild(wrap);
        parent.appendChild(checks);
    }

    function addTextField(parent, owner, field) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, field.label));
        var input = document.createElement('input');
        input.type = 'text';
        input.spellcheck = false;
        input.value = owner[field.key] == null ? '' : String(owner[field.key]);
        input.addEventListener('input', function () { owner[field.key] = input.value; });
        wrap.appendChild(input);
        parent.appendChild(wrap);
    }

    function addNumberField(parent, owner, field) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, field.label));
        var input = document.createElement('input');
        input.type = 'number';
        input.step = 'any';
        if (field.placeholder) input.placeholder = field.placeholder;
        if (owner[field.key] != null && Number.isFinite(Number(owner[field.key]))) {
            input.value = formatNum(Number(owner[field.key]));
        }
        input.addEventListener('input', function () {
            if (input.value === '' && field.optional) {
                delete owner[field.key];
                return;
            }
            var value = parseFloat(input.value);
            if (Number.isFinite(value)) owner[field.key] = value;
        });
        wrap.appendChild(input);
        parent.appendChild(wrap);
    }

    function toHex(rgb) {
        return '#' + [0, 1, 2].map(function (i) {
            var n = Math.max(0, Math.min(255, Math.round((Number(rgb[i]) || 0) * 255)));
            return n.toString(16).padStart(2, '0');
        }).join('');
    }

    // A fixed length array of numbers stored at owner[key].
    function numberArray(owner, key, length) {
        if (!Array.isArray(owner[key])) owner[key] = [];
        while (owner[key].length < length) owner[key].push(0);
        return owner[key];
    }

    function addVecInputs(parent, values, names, picker) {
        var row = el('div', 'vec');
        var inputs = [];
        var colorInput = null;
        names.forEach(function (name, i) {
            var wrap = el('label', null, name);
            var input = document.createElement('input');
            input.type = 'number';
            input.step = 'any';
            input.value = formatNum(Number(values[i]) || 0);
            input.addEventListener('input', function () {
                var value = parseFloat(input.value);
                if (!Number.isFinite(value)) return;
                values[i] = value;
                if (colorInput) colorInput.value = toHex(values);
            });
            wrap.appendChild(input);
            row.appendChild(wrap);
            inputs.push(input);
        });
        if (picker) {
            var line = el('div', 'color-row');
            colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = toHex(values);
            colorInput.addEventListener('input', function () {
                var n = parseInt(colorInput.value.slice(1), 16);
                values[0] = ((n >> 16) & 255) / 255;
                values[1] = ((n >> 8) & 255) / 255;
                values[2] = (n & 255) / 255;
                inputs.forEach(function (input, i) { input.value = formatNum(values[i]); });
            });
            line.appendChild(colorInput);
            line.appendChild(row);
            parent.appendChild(line);
        } else {
            parent.appendChild(row);
        }
    }

    function addVecField(parent, owner, field) {
        var wrap = el('div', 'field');
        if (field.optional) {
            addCheck(wrap, field.label, Array.isArray(owner[field.key]), function (on) {
                if (on) owner[field.key] = clone(field.initial);
                else delete owner[field.key];
                rerender();
            });
            if (!Array.isArray(owner[field.key])) {
                parent.appendChild(wrap);
                return;
            }
        } else {
            wrap.appendChild(el('span', null, field.label));
        }
        addVecInputs(wrap, numberArray(owner, field.key, field.names.length), field.names, field.picker);
        parent.appendChild(wrap);
    }

    function addScaleFields(parent, deco) {
        var perAxis = Array.isArray(deco.min) || Array.isArray(deco.max);
        var wrap = el('div', 'field');
        addCheck(wrap, 'Scale each axis separately', perAxis, function (on) {
            ['min', 'max'].forEach(function (key) {
                var value = deco[key];
                var single = Array.isArray(value) ? Number(value[0]) : Number(value);
                if (!Number.isFinite(single)) single = 1;
                deco[key] = on ? [single, single, single] : single;
            });
            rerender();
        });
        parent.appendChild(wrap);
        [['min', 'Min scale'], ['max', 'Max scale']].forEach(function (entry) {
            if (perAxis) {
                var vecWrap = el('div', 'field');
                vecWrap.appendChild(el('span', null, entry[1]));
                addVecInputs(vecWrap, numberArray(deco, entry[0], 3), ['x', 'y', 'z'], false);
                parent.appendChild(vecWrap);
            } else {
                addNumberField(parent, deco, { key: entry[0], label: entry[1] });
            }
        });
    }

    function addProfileFields(parent, deco) {
        var usesSvg = typeof deco.svg === 'string';
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, 'Profile'));
        var select = document.createElement('select');
        [['profile', 'Points'], ['svg', 'SVG file']].forEach(function (entry) {
            var option = document.createElement('option');
            option.value = entry[0];
            option.textContent = entry[1];
            select.appendChild(option);
        });
        select.value = usesSvg ? 'svg' : 'profile';
        select.addEventListener('change', function () {
            if (select.value === 'svg') {
                if (typeof deco.svg !== 'string') deco.svg = '';
            } else {
                delete deco.svg;
                if (!Array.isArray(deco.profile)) deco.profile = clone(specFor('edge', 'extrude').defaults.profile);
            }
            rerender();
        });
        wrap.appendChild(select);
        parent.appendChild(wrap);

        if (usesSvg) {
            addTextField(parent, deco, { key: 'svg', label: 'SVG name (square loads square.svg)' });
            return;
        }
        if (!Array.isArray(deco.profile)) deco.profile = [];
        var table = el('table', 'half-table');
        var head = el('tr');
        ['#', 'x', 'y', ''].forEach(function (name) { head.appendChild(el('th', null, name)); });
        table.appendChild(head);
        deco.profile.forEach(function (point, index) {
            if (!Array.isArray(point)) {
                point = [0, 0];
                deco.profile[index] = point;
            }
            numberArray(deco.profile, index, 2);
            var row = el('tr');
            row.appendChild(el('td', null, String(index)));
            [0, 1].forEach(function (axis) {
                var cell = el('td');
                var input = document.createElement('input');
                input.type = 'number';
                input.step = 'any';
                input.value = formatNum(Number(point[axis]) || 0);
                input.addEventListener('input', function () {
                    var value = parseFloat(input.value);
                    if (Number.isFinite(value)) point[axis] = value;
                });
                cell.appendChild(input);
                row.appendChild(cell);
            });
            var removeCell = el('td');
            var remove = el('button', 'small', 'Remove');
            remove.type = 'button';
            remove.addEventListener('click', function () {
                deco.profile.splice(index, 1);
                rerender();
            });
            removeCell.appendChild(remove);
            row.appendChild(removeCell);
            table.appendChild(row);
        });
        parent.appendChild(table);
        var add = el('button', 'small', 'Add point');
        add.type = 'button';
        add.addEventListener('click', function () {
            var last = deco.profile[deco.profile.length - 1];
            deco.profile.push(Array.isArray(last) ? [last[0], last[1]] : [0, 0]);
            rerender();
        });
        parent.appendChild(add);
        if (deco.profile.length < 3) addNote(parent, 'A profile needs at least 3 points.', 'note warn-text');
    }

    // ----- Choosing a decoration ----------------------------------------
    // ownerId is the decoration that will hold the choice, or null when a
    // type holds it. A decoration is never offered inside itself.
    // placeholder is the label for "nothing chosen", or null for no such option.
    // withNew adds an "Add new..." option that creates a decoration and picks it.
    function buildSelect(refKind, current, ownerKind, ownerId, placeholder, onPick, withNew) {
        var select = document.createElement('select');
        function addOption(value, text) {
            var option = document.createElement('option');
            option.value = value;
            option.textContent = text;
            select.appendChild(option);
        }
        if (placeholder) addOption('', placeholder);
        var found = false;
        list(refKind).forEach(function (deco) {
            var isCurrent = deco.id === current;
            if (isCurrent) found = true;
            if (!isCurrent && ownerId != null && refKind === ownerKind && wouldCycle(refKind, ownerId, deco.id)) return;
            addOption('id:' + deco.id, deco.id);
        });
        if (current && !found) addOption('id:' + current, current + ' (missing)');
        if (withNew) addOption('new', 'Add new\u2026');
        select.value = current ? 'id:' + current : '';
        select.addEventListener('change', function () {
            if (select.value === 'new') onPick(createDecoration(refKind));
            else if (select.value === '') onPick(null);
            else onPick(select.value.slice(3));
            rerender();
        });
        return select;
    }

    // A button with an arrow pointing up and to the right. It opens the panel
    // devoted to one decoration. Nothing is returned if there is no such decoration.
    function editButton(kind, id) {
        if (!find(kind, id)) return null;
        var button = el('button', 'small icon', '\u2197');
        button.type = 'button';
        button.title = 'Edit the ' + kind + ' decoration "' + id + '"';
        button.setAttribute('aria-label', button.title);
        button.addEventListener('click', function () { openDecoration(kind, id); });
        return button;
    }

    function addRefField(parent, kind, deco, field) {
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, field.label));
        var current = typeof deco[field.key] === 'string' ? deco[field.key] : '';
        var row = el('div', 'deco-child-row');
        wrap.appendChild(row);
        row.appendChild(buildSelect(
            field.refKind, current, kind, deco.id,
            field.optional ? 'None' : 'Choose\u2026',
            function (id) {
                if (id == null) delete deco[field.key];
                else deco[field.key] = id;
            },
            true
        ));
        parent.appendChild(wrap);
        if (!current && !field.optional) addNote(parent, 'Required.', 'note warn-text');
        var edit = current ? editButton(field.refKind, current) : null;
        if (edit) row.appendChild(edit);
    }

    // How many empty child rows each decoration is showing, by "kind:id".
    var pendingRows = {};

    function addRefListField(parent, kind, deco, field) {
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, field.label));
        parent.appendChild(wrap);
        if (field.weights) {
            addCheck(wrap, 'Custom weights', Array.isArray(deco.weight), function (on) {
                if (on) deco.weight = (deco.childIds || []).map(function () { return 1; });
                else delete deco.weight;
                rerender();
            });
        }
        var ids = deco.childIds || [];
        ids.forEach(function (childId, index) {
            var row = el('div', 'deco-child-row');
            row.appendChild(buildSelect(field.refKind, childId, kind, deco.id, null, function (id) {
                deco.childIds[index] = id;
            }, true));
            if (Array.isArray(deco.weight)) {
                var weight = document.createElement('input');
                weight.type = 'number';
                weight.step = 'any';
                weight.title = 'Weight';
                weight.value = formatNum(Number(deco.weight[index]));
                weight.addEventListener('input', function () {
                    var value = parseFloat(weight.value);
                    if (!Number.isFinite(value)) return;
                    while (deco.weight.length <= index) deco.weight.push(1);
                    deco.weight[index] = value;
                });
                row.appendChild(weight);
            }
            var edit = editButton(field.refKind, childId);
            if (edit) row.appendChild(edit);
            var remove = el('button', 'small icon', 'X');
            remove.type = 'button';
            remove.title = 'Remove this child';
            remove.setAttribute('aria-label', 'Remove this child');
            remove.addEventListener('click', function () {
                deco.childIds.splice(index, 1);
                if (Array.isArray(deco.weight)) deco.weight.splice(index, 1);
                rerender();
            });
            row.appendChild(remove);
            parent.appendChild(row);
        });
        // Rows the user has added with the plus button and not filled in yet.
        // They are not in the file until a decoration is chosen for them.
        var pendingKey = kind + ':' + deco.id;
        for (var p = 0; p < (pendingRows[pendingKey] || 0); p++) {
            var pendingRow = el('div', 'deco-child-row');
            pendingRow.appendChild(buildSelect(field.refKind, '', kind, deco.id, null, function (id) {
                pendingRows[pendingKey]--;
                if (!Array.isArray(deco.childIds)) deco.childIds = [];
                deco.childIds.push(id);
                if (Array.isArray(deco.weight)) deco.weight.push(1);
            }, true));
            var cancel = el('button', 'small icon', 'X');
            cancel.type = 'button';
            cancel.title = 'Remove this row';
            cancel.setAttribute('aria-label', 'Remove this row');
            cancel.addEventListener('click', function () {
                pendingRows[pendingKey]--;
                rerender();
            });
            pendingRow.appendChild(cancel);
            parent.appendChild(pendingRow);
        }
        var addRow = el('div', 'deco-child-row');
        var plus = el('button', 'small plus', '+');
        plus.type = 'button';
        plus.title = 'Add a child';
        plus.setAttribute('aria-label', 'Add a child');
        plus.addEventListener('click', function () {
            pendingRows[pendingKey] = (pendingRows[pendingKey] || 0) + 1;
            rerender();
        });
        addRow.appendChild(plus);
        parent.appendChild(addRow);
        if (Array.isArray(deco.weight) && deco.weight.length !== ids.length) {
            addNote(parent, 'There are ' + deco.weight.length + ' weights for ' + ids.length + ' children.', 'note warn-text');
        }
    }

    // ----- One decoration -----------------------------------------------
    function addTypeSelect(parent, kind, deco) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, 'Decoration type'));
        var select = document.createElement('select');
        TYPES[kind].forEach(function (spec) {
            var option = document.createElement('option');
            option.value = spec.type;
            option.textContent = spec.label;
            select.appendChild(option);
        });
        if (!specFor(kind, deco.type)) {
            var unknown = document.createElement('option');
            unknown.value = String(deco.type);
            unknown.textContent = String(deco.type) + ' (unknown)';
            select.appendChild(unknown);
        }
        select.value = String(deco.type);
        select.addEventListener('change', function () {
            changeType(kind, deco, select.value);
            rerender();
        });
        wrap.appendChild(select);
        parent.appendChild(wrap);
    }

    function addIdField(parent, kind, deco) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, 'Id'));
        var input = document.createElement('input');
        input.type = 'text';
        input.spellcheck = false;
        input.value = deco.id;
        wrap.appendChild(input);
        var message = el('p', 'note warn-text');
        message.hidden = true;
        wrap.appendChild(message);
        input.addEventListener('change', function () {
            var next = input.value.trim();
            message.hidden = true;
            if (next === deco.id) {
                input.value = deco.id;
                return;
            }
            var problem = !next ? 'An id cannot be empty.'
                : find(kind, next) ? 'Another ' + kind + ' decoration already has that id.'
                : '';
            if (problem) {
                input.value = deco.id;
                message.textContent = problem;
                message.hidden = false;
                return;
            }
            renameDecoration(kind, deco, next);
            rerender();
        });
        parent.appendChild(wrap);
    }

    function addExtraFields(parent, deco, known) {
        var extra = {};
        var found = false;
        Object.keys(deco).forEach(function (key) {
            if (known.indexOf(key) >= 0) return;
            extra[key] = deco[key];
            found = true;
        });
        if (!found) return;
        addNote(parent, 'Other fields are kept in the file:');
        parent.appendChild(el('pre', 'extra', JSON.stringify(extra, null, 2)));
    }

    // ----- The decoration panel -----------------------------------------
    // A decoration is not part of a type, so its panel is a selection of its
    // own. The editor keeps it in its selection history, and the history
    // arrows are the way back.

    // Fills parent with the panel for one decoration.
    var shownPanel = '';

    function renderPanel(parent, kind, id) {
        // Empty child rows belong to the decoration they were added in.
        if (shownPanel !== kind + ':' + id) pendingRows = {};
        shownPanel = kind + ':' + id;
        var deco = find(kind, id);
        if (!deco) {
            addNote(parent, 'There is no ' + kind + ' decoration with the id "' + id + '".', 'note warn-text');
            return;
        }
        var box = el('div', 'decoration');
        parent.appendChild(box);

        addIdField(box, kind, deco);
        addTypeSelect(box, kind, deco);

        var spec = specFor(kind, deco.type);
        var known = ['id', 'type'];
        if (spec) {
            spec.fields.forEach(function (field) {
                fieldKeys(field).forEach(function (key) { known.push(key); });
                if (field.control === 'text') addTextField(box, deco, field);
                else if (field.control === 'number') addNumberField(box, deco, field);
                else if (field.control === 'vec') addVecField(box, deco, field);
                else if (field.control === 'scale') addScaleFields(box, deco);
                else if (field.control === 'profile') addProfileFields(box, deco);
                else if (field.control === 'ref') addRefField(box, kind, deco, field);
                else if (field.control === 'refList') addRefListField(box, kind, deco, field);
            });
            var warning = spec.warning ? spec.warning(deco) : '';
            if (warning) addNote(box, warning, 'note warn-text');
        } else {
            addNote(box, 'This decoration type is not known to the editor.', 'note warn-text');
        }
        addExtraFields(box, deco, known);

        var remove = el('button', 'small', 'Delete decoration');
        remove.type = 'button';
        remove.title = 'Removes this decoration everywhere it is used. Ctrl+Z undoes it.';
        remove.addEventListener('click', function () {
            deleteDecoration(kind, deco);
            goBack();
        });
        box.appendChild(remove);
    }

    // ----- Entry points -------------------------------------------------
    // kind is 'vertex', 'edge', or 'face'. type is the type being edited.
    function render(parent, kind, type) {
        var box = el('fieldset');
        box.appendChild(el('legend', null, 'Decoration'));
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, 'Assigned ' + kind + ' decoration'));
        var current = typeof type.decoration === 'string' ? type.decoration : '';
        var row = el('div', 'deco-child-row');
        row.appendChild(buildSelect(kind, current, kind, null, 'None', function (id) {
            if (id == null) delete type.decoration;
            else type.decoration = id;
        }, true));
        var edit = current ? editButton(kind, current) : null;
        if (edit) row.appendChild(edit);
        wrap.appendChild(row);
        box.appendChild(wrap);
        if (current && !edit) {
            addNote(box, 'There is no ' + kind + ' decoration with the id "' + current + '".', 'note warn-text');
        }
        parent.appendChild(box);
    }

    // options:
    //   doc()        the document being edited
    //   rerender()   redraw the panel after a change
    //   open(kind, id)      select a decoration, so its panel is shown
    //   back()       step back in the selection history
    //   renamed(kind, oldId, newId)   a decoration was renamed
    var openDecoration = function () {};
    var goBack = function () {};
    var onRenamed = function () {};

    function init(options) {
        getDoc = options.doc;
        rerender = options.rerender;
        openDecoration = options.open;
        goBack = options.back;
        onRenamed = options.renamed;
    }

    window.PrimitivesDecorations = {
        init: init,
        render: render,
        renderPanel: renderPanel,
        kinds: KINDS,
    };
})();
