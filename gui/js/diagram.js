import { state } from './state.js';

export function drawCircuitDiagram(netlist) {
    console.log('Drawing circuit with netlist:', netlist);

    const svg = document.getElementById('circuitDiagram');
    if (!svg || netlist.length === 0) return;

    while (svg.firstChild) svg.removeChild(svg.firstChild);

    const width = svg.clientWidth || 1400;
    const height = svg.clientHeight || 800;

    const bg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    bg.setAttribute('width', width);
    bg.setAttribute('height', height);
    bg.setAttribute('fill', '#fafafa');
    svg.appendChild(bg);

    const gatesByOutput = {};
    netlist.forEach(gate => {
        gatesByOutput[gate.output] = gate;
    });

    const allOutputs = new Set(netlist.map(g => g.output));
    const primaryInputs = new Set();
    netlist.forEach(gate => {
        gate.inputs.forEach(inp => {
            if (!allOutputs.has(inp)) {
                primaryInputs.add(inp);
            }
        });
    });
    const inputs = Array.from(primaryInputs).sort();
    const outputs = Array.from(allOutputs).sort();
    const allWires = new Set(inputs.concat(Array.from(allOutputs)));

    const railsGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    railsGroup.setAttribute('id', 'rails-group');
    svg.appendChild(railsGroup);

    const wiresGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    wiresGroup.setAttribute('id', 'wires-group');
    svg.appendChild(wiresGroup);

    const gatesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    gatesGroup.setAttribute('id', 'gates-group');
    svg.appendChild(gatesGroup);

    const portsGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    portsGroup.setAttribute('id', 'ports-group');
    svg.appendChild(portsGroup);

    const depthMemo = {};
    const calculateDepth = (wire) => {
        if (wire in depthMemo) return depthMemo[wire];
        if (!gatesByOutput[wire]) {
            depthMemo[wire] = 0;
            return 0;
        }
        const gate = gatesByOutput[wire];
        const inputsDepths = gate.inputs.map(i => calculateDepth(i));
        const d = Math.max(...inputsDepths) + 1;
        depthMemo[wire] = d;
        return d;
    };
    outputs.forEach(o => calculateDepth(o));

    const maxDepth = Math.max(0, ...Object.values(depthMemo));

    const cols = [];
    const totalCols = Math.max(1, maxDepth + 2);
    const colSpacing = (width - 140) / (totalCols - 1);
    for (let c = 0; c < totalCols; c++) cols.push(60 + c * colSpacing);

    const wireY = {};
    const topMargin = 60;
    const bottomMargin = 60;
    const availableHeight = Math.max(200, height - topMargin - bottomMargin);
    const inputSpacing = availableHeight / (Math.max(1, inputs.length) + 1);
    inputs.forEach((inp, idx) => {
        wireY[inp] = topMargin + (idx + 1) * inputSpacing;
    });

    const gatesByDepth = {};
    netlist.forEach(g => {
        const d = depthMemo[g.output] || 0;
        if (!gatesByDepth[d]) gatesByDepth[d] = [];
        gatesByDepth[d].push(g);
    });

    for (let d = 1; d <= maxDepth; d++) {
        const gates = gatesByDepth[d] || [];
        const blockTop = topMargin;
        const blockHeight = availableHeight;
        const spacing = blockHeight / (Math.max(1, gates.length) + 1);
        gates.forEach((g, i) => {
            const y = blockTop + (i + 1) * spacing;
            wireY[g.output] = y;
        });
    }

    const wireProducerX = {};
    inputs.forEach(i => wireProducerX[i] = cols[0]);
    Object.keys(depthMemo).forEach(w => {
        const d = depthMemo[w] || 0;
        wireProducerX[w] = cols[Math.min(d, cols.length - 1)];
    });

    netlist.forEach(gate => {
        if (gate.type === 'IDENTITY' && gate.inputs.length === 1) {
            const inputWire = gate.inputs[0];
            const outputWire = gate.output;
            if (wireY[inputWire] !== undefined) {
                wireY[outputWire] = wireY[inputWire];
                wireProducerX[outputWire] = wireProducerX[inputWire];
            }
        }
    });

    let fallbackY = topMargin + availableHeight + 20;
    Array.from(allWires).forEach(w => {
        if (!wireY[w]) {
            wireY[w] = fallbackY;
            fallbackY += 24;
        }
    });

    // חישוב נקודות הפיצול (taps) לכל חוט
    const wireTapPositions = {};
    Array.from(allWires).forEach(w => {
        wireTapPositions[w] = [];
    });

    const depthLaneTracker = {};
    netlist.forEach(gate => {
        if (gate.type === 'IDENTITY') return;
        const d = depthMemo[gate.output] || 0;
        const gX = cols[Math.min(d, cols.length - 1)];

        const LANE_SPACING = 30;
        gate.inputs.forEach((inp, idx) => {
            if (!depthLaneTracker[d]) depthLaneTracker[d] = 0;
            const laneIndex = depthLaneTracker[d]++;
            const totalOffsetX = 20 + (laneIndex * LANE_SPACING);
            const baseTapX = gX - 40;
            const railTapX = baseTapX - totalOffsetX;
            
            wireTapPositions[inp].push(railTapX);
        });
    });

    // חישוב נקודת הסיום של כל rail - הנקודה הימנית ביותר מבין כל ה-taps שלו
    const wireConsumerX = {};
    Array.from(allWires).forEach(w => {
        let maxX = wireProducerX[w];
        if (outputs.includes(w)) {
            maxX = cols[cols.length - 1];
        } else if (wireTapPositions[w].length > 0) {
            // הנקודה הימנית ביותר היא ה-tap עם ה-X המקסימלי
            maxX = Math.max(...wireTapPositions[w]);
        }
        wireConsumerX[w] = maxX;
    });

    const palette = ['#6a1b9a', '#00897b', '#f57c00', '#43a047', '#1e88e5', '#fb8c00', '#8e24aa', '#e53935', '#c62828', '#00695c', '#bf360c', '#1b5e20', '#ad1457', '#00838f', '#e65100', '#2e7d32', '#283593', '#f57f17'];
    const segmentColor = {};
    let colorIdx = 0;

    inputs.forEach(inp => {
        segmentColor[inp] = palette[colorIdx % palette.length];
        colorIdx++;
    });

    netlist.forEach(gate => {
        segmentColor[gate.output] = palette[colorIdx % palette.length];
        colorIdx++;
    });

    const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
    const marker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
    marker.setAttribute('id', 'arrow');
    marker.setAttribute('markerWidth', '8');
    marker.setAttribute('markerHeight', '8');
    marker.setAttribute('refX', '6');
    marker.setAttribute('refY', '3');
    marker.setAttribute('orient', 'auto');
    const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    path.setAttribute('d', 'M0,0 L6,3 L0,6 z');
    path.setAttribute('fill', '#333');
    marker.appendChild(path);
    defs.appendChild(marker);
    svg.appendChild(defs);

    Array.from(allWires).forEach(w => {
        const y = wireY[w];
        
        // הוספת התיקון להתחלת חוט הקלט - מזיזים ימינה כדי לא לכסות את הטקסט
        const isPrimaryInput = !gatesByOutput[w];
        const xStart = isPrimaryInput ? 55 : wireProducerX[w] + 12;

        const xEnd = wireConsumerX[w];

        if (xEnd > xStart) {
            const rail = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            rail.setAttribute('x1', xStart);
            rail.setAttribute('y1', y);
            rail.setAttribute('x2', xEnd);
            rail.setAttribute('y2', y);
            rail.setAttribute('stroke', segmentColor[w]);
            rail.setAttribute('stroke-width', '3');
            rail.setAttribute('stroke-linecap', 'round');
            rail.setAttribute('data-wire', w);
            // rail.classList.add('wire-glow-0');
            railsGroup.appendChild(rail);
        }
    });

    function connectRailToGate(wireName, gateX, gateInputY, offsetX = 0) {
        const railY = wireY[wireName];
        const baseTapX = gateX - 40;
        const railTapX = baseTapX - offsetX;
        const color = segmentColor[wireName] || '#999';

        const v = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        v.setAttribute('x1', railTapX);
        v.setAttribute('y1', railY);
        v.setAttribute('x2', railTapX);
        v.setAttribute('y2', gateInputY);
        v.setAttribute('stroke', color);
        v.setAttribute('stroke-width', '3');
        v.setAttribute('stroke-linecap', 'round');
        v.setAttribute('data-wire', wireName);
        // v.classList.add('wire-glow-0');
        wiresGroup.appendChild(v);

        const h2 = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        h2.setAttribute('x1', railTapX);
        h2.setAttribute('y1', gateInputY);
        h2.setAttribute('x2', gateX - 18);
        h2.setAttribute('y2', gateInputY);
        h2.setAttribute('stroke', color);
        h2.setAttribute('stroke-width', '3');
        h2.setAttribute('data-wire', wireName);
        // h2.classList.add('wire-glow-0');
        wiresGroup.appendChild(h2);

        const tap = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        tap.setAttribute('cx', railTapX);
        tap.setAttribute('cy', railY);
        tap.setAttribute('r', '4');
        tap.setAttribute('fill', color);
        tap.setAttribute('stroke', '#fff');
        tap.setAttribute('stroke-width', '1.5');
        wiresGroup.appendChild(tap);
    }

    // איפוס depthLaneTracker לשימוש בציור בפועל
    Object.keys(depthLaneTracker).forEach(k => depthLaneTracker[k] = 0);

    netlist.forEach(gate => {
        if (gate.type === 'IDENTITY') {
            return;
        }
        const d = depthMemo[gate.output] || 0;
        const gX = cols[Math.min(d, cols.length - 1)];
        const gY = wireY[gate.output];

        const LANE_SPACING = 30;

        gate.inputs.forEach((inp, idx) => {
            const inputCount = gate.inputs.length;
            const inputY = gY - 10 + (inputCount === 1 ? 10 : (idx * (20 / Math.max(1, inputCount - 1))));

            if (!depthLaneTracker[d]) depthLaneTracker[d] = 0;
            const laneIndex = depthLaneTracker[d]++;
            const totalOffsetX = 20 + (laneIndex * LANE_SPACING);

            connectRailToGate(inp, gX, inputY, totalOffsetX);
        });

        drawGateSymbol(gatesGroup, gX - 22, gY - 18, gate.type, gate.output);

        const outStub = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        outStub.setAttribute('x1', gX + 25);
        outStub.setAttribute('y1', gY);
        outStub.setAttribute('x2', gX + 40);
        outStub.setAttribute('y2', gY);
        outStub.setAttribute('stroke', segmentColor[gate.output]);
        outStub.setAttribute('stroke-width', '3');
        outStub.setAttribute('data-wire', gate.output);
        outStub.classList.add('wire-glow-0');
        wiresGroup.appendChild(outStub);
    });

    inputs.forEach(i => {
        const y = wireY[i];
        const txt = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        txt.setAttribute('x', 50);
        txt.setAttribute('y', y + 5);
        txt.setAttribute('text-anchor', 'end');
        txt.setAttribute('font-size', '13');
        txt.setAttribute('font-weight', 'bold');
        txt.setAttribute('fill', '#1976d2');
        txt.textContent = i;
        portsGroup.appendChild(txt);
        
        const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        dot.setAttribute('cx', 55);
        dot.setAttribute('cy', y);
        dot.setAttribute('r', '4');
        dot.setAttribute('fill', segmentColor[i]);
        dot.setAttribute('stroke', '#fff');
        dot.setAttribute('stroke-width', '1.5');
        portsGroup.appendChild(dot);
    });

    outputs.forEach(o => {
        const y = wireY[o];
        const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        dot.setAttribute('cx', cols[cols.length - 1]);
        dot.setAttribute('cy', y);
        dot.setAttribute('r', '6');
        dot.setAttribute('fill', '#66bb6a');
        dot.setAttribute('stroke', '#2e7d32');
        dot.setAttribute('stroke-width', '2');
        portsGroup.appendChild(dot);

        const txt = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        txt.setAttribute('x', cols[cols.length - 1] + 14);
        txt.setAttribute('y', y + 5);
        txt.setAttribute('text-anchor', 'start');
        txt.setAttribute('font-size', '13');
        txt.setAttribute('font-weight', 'bold');
        txt.setAttribute('fill', '#2e7d32');
        txt.textContent = o;
        portsGroup.appendChild(txt);
    });

    console.log('✓ Circuit diagram complete');
}

function drawGateSymbol(layer, x, y, gateType, label) {
    const colors = {
        'AND': { fill: '#4CAF50', icon: 'AND' },
        'OR': { fill: '#2196F3', icon: 'OR' },
        'XOR': { fill: '#FF9800', icon: 'XOR' },
        'NOT': { fill: '#F44336', icon: 'NOT' },
        'NAND': { fill: '#9C27B0', icon: 'NAND' },
        'NOR': { fill: '#00BCD4', icon: 'NOR' },
        'IDENTITY': { fill: '#607D8B', icon: '=' },
        'CONSTANT': { fill: '#795548', icon: 'C' }
    };

    const gateColor = colors[gateType] || { fill: '#2a5298', icon: '?' };

    switch (gateType) {
        case 'AND':
            const andPath = `M ${x} ${y} L ${x + 50} ${y} Q ${x + 70} ${y + 25} ${x + 50} ${y + 50} L ${x} ${y + 50} Z`;
            const andGate = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            andGate.setAttribute('d', andPath);
            andGate.setAttribute('fill', gateColor.fill);
            andGate.setAttribute('stroke', '#333');
            andGate.setAttribute('stroke-width', '2');
            layer.appendChild(andGate);
            break;

        case 'OR':
            const orPath = `M ${x + 5} ${y} Q ${x + 15} ${y + 25} ${x + 5} ${y + 50} L ${x + 50} ${y + 50} Q ${x + 70} ${y + 25} ${x + 50} ${y} Z`;
            const orGate = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            orGate.setAttribute('d', orPath);
            orGate.setAttribute('fill', gateColor.fill);
            orGate.setAttribute('stroke', '#333');
            orGate.setAttribute('stroke-width', '2');
            layer.appendChild(orGate);
            break;

        case 'XOR':
            const xorPath = `M ${x + 5} ${y} Q ${x + 15} ${y + 25} ${x + 5} ${y + 50} L ${x + 50} ${y + 50} Q ${x + 70} ${y + 25} ${x + 50} ${y} Z`;
            const xorGate = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            xorGate.setAttribute('d', xorPath);
            xorGate.setAttribute('fill', gateColor.fill);
            xorGate.setAttribute('stroke', '#333');
            xorGate.setAttribute('stroke-width', '2');
            layer.appendChild(xorGate);
            const xorLine = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            xorLine.setAttribute('d', `M ${x + 2} ${y} Q ${x + 8} ${y + 25} ${x + 2} ${y + 50}`);
            xorLine.setAttribute('stroke', '#333');
            xorLine.setAttribute('stroke-width', '2');
            xorLine.setAttribute('fill', 'none');
            layer.appendChild(xorLine);
            break;

        case 'NOT':
            const notPath = `M ${x} ${y} L ${x} ${y + 50} L ${x + 50} ${y + 25} Z`;
            const notGate = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            notGate.setAttribute('d', notPath);
            notGate.setAttribute('fill', gateColor.fill);
            notGate.setAttribute('stroke', '#333');
            notGate.setAttribute('stroke-width', '2');
            layer.appendChild(notGate);
            const notCircle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
            notCircle.setAttribute('cx', x + 60);
            notCircle.setAttribute('cy', y + 25);
            notCircle.setAttribute('r', '6');
            notCircle.setAttribute('fill', 'none');
            notCircle.setAttribute('stroke', '#333');
            notCircle.setAttribute('stroke-width', '2');
            layer.appendChild(notCircle);
            break;

        default:
            const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            rect.setAttribute('x', x);
            rect.setAttribute('y', y);
            rect.setAttribute('width', 60);
            rect.setAttribute('height', 50);
            rect.setAttribute('rx', '5');
            rect.setAttribute('fill', gateColor.fill);
            rect.setAttribute('stroke', '#333');
            rect.setAttribute('stroke-width', '2');
            layer.appendChild(rect);
    }

    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('x', x + 30);
    text.setAttribute('y', y + 30);
    text.setAttribute('text-anchor', 'middle');
    text.setAttribute('font-weight', 'bold');
    text.setAttribute('font-size', '12');
    text.setAttribute('fill', 'white');
    text.textContent = gateColor.icon;
    layer.appendChild(text);
}