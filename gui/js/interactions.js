import { state, setHighlightedWire } from './state.js';

const tooltip = document.getElementById('wire-tooltip');

if (tooltip) {
    tooltip.onmouseover = () => {
        clearTimeout(state.hideTimeout);
    };
    tooltip.onmouseout = () => {
        state.hideTimeout = setTimeout(hideTooltip, 100);
    };
}

export function setVerilogData(data) {
    if (!data || !data.values) {
        console.error("Invalid data received from simulator (missing values).");
        return;
    }
    state.wireValues = data.values;
    state.wireExpressions = data.expressions || {};
    
    applyStylingAndInteractions(state.wireValues);
}

function applyStylingAndInteractions(values) {
    const svg = document.getElementById('circuitDiagram');
    if (!svg) return;

    let needsReHighlight = null;
    if (state.highlightedWireName) {
        needsReHighlight = state.highlightedWireName;
        state.highlightedWireName = null;
    }

    svg.querySelectorAll('[data-value-label]').forEach(el => el.remove());

    svg.querySelectorAll('#rails-group line, #wires-group line').forEach(line => {
        const wireName = line.getAttribute('data-wire');
        if (wireName && values.hasOwnProperty(wireName)) {
            const value = values[wireName];

            line.classList.remove('wire-glow-0', 'wire-glow-1');
            line.classList.add(value ? 'wire-glow-1' : 'wire-glow-0');
            line.classList.remove('wire-highlight');

            line.onmouseover = (e) => showTooltip(e, wireName);
            line.onmousemove = (e) => moveTooltip(e);
            line.onmouseout = () => {
                state.hideTimeout = setTimeout(hideTooltip, 100);
            };

            line.onclick = (e) => handleWireClick(wireName);
        }
    });

    if (needsReHighlight) {
        svg.querySelectorAll(`#rails-group line[data-wire='${needsReHighlight}'], #wires-group line[data-wire='${needsReHighlight}']`).forEach(line => {
            line.classList.add('wire-highlight');
        });
        state.highlightedWireName = needsReHighlight;
    }
}

export function handleWireClick(wireName) {
    const svg = document.getElementById('circuitDiagram');
    svg.querySelectorAll('.wire-highlight').forEach(line => {
        line.classList.remove('wire-highlight');
    });

    if (state.highlightedWireName === wireName) {
        state.highlightedWireName = null;
    } else {
        svg.querySelectorAll(`#rails-group line[data-wire='${wireName}'], #wires-group line[data-wire='${wireName}']`).forEach(line => {
            line.classList.add('wire-highlight');
        });
        state.highlightedWireName = wireName;
    }
}

function showTooltip(event, wireName) {
    let expr = state.wireExpressions[wireName] || "אין ביטוי זמין. יש לעדכן את קוד ה-C++ (Wasm).";
    let valueStr = state.wireValues[wireName] !== undefined ? state.wireValues[wireName] : '?';
    let tooltipContent = `<b>Wire:</b> ${wireName}<br><b>Value:</b> ${valueStr}<hr style="margin: 5px 0; border-color: #3498db;"><b>Logical expression:</b> `;

    let styledExpr = expr.replace(/([A-Za-z0-9_]+)/g, (match) => {
        if (state.wireValues[match] !== undefined && match !== wireName) {
            return `<span class="expr-var" data-var-name="${match}">${match}</span>`;
        }
        return match;
    });

    tooltip.innerHTML = tooltipContent + styledExpr;
    tooltip.style.visibility = 'visible';
    moveTooltip(event);

    tooltip.querySelectorAll('.expr-var').forEach(span => {
        span.onmouseover = (e) => showVarValue(e, span.getAttribute('data-var-name'));
        span.onmouseout = () => hideVarValue();
    });
}

function showVarValue(event, varName) {
    if (state.varTooltip) state.varTooltip.remove();

    const value = state.wireValues[varName] !== undefined ? state.wireValues[varName] : 'N/A';

    state.varTooltip = document.createElement('div');
    state.varTooltip.style.cssText = `
        position: fixed; 
        background-color: #34495e;
        color: #ecf0f1;
        padding: 4px 8px;
        border-radius: 3px;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.5);
        font-size: 0.8em;
        z-index: 1001;
        white-space: nowrap;
        pointer-events: none;
    `;
    state.varTooltip.innerHTML = `<b>${varName}</b>: ${value}`;
    document.body.appendChild(state.varTooltip);

    state.varTooltip.style.left = (event.clientX + 15) + 'px';
    state.varTooltip.style.top = (event.clientY - 25) + 'px';
}

function moveTooltip(event) {
    tooltip.style.left = (event.clientX + 10) + 'px';
    tooltip.style.top = (event.clientY + 10) + 'px';
}

function hideTooltip() {
    tooltip.style.visibility = 'hidden';
    hideVarValue();
    tooltip.querySelectorAll('.expr-var').forEach(span => {
        span.onmouseover = null;
        span.onmouseout = null;
    });
}

function hideVarValue() {
    if (state.varTooltip) {
        state.varTooltip.remove();
        state.varTooltip = null;
    }
}