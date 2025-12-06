// Global State Management
export const state = {
    Module: null,
    wireValues: {},
    wireExpressions: {},
    highlightedWireName: null,
    inputValues: {},
    varTooltip: null,
    hideTimeout: null
};

export function setModule(m) {
    state.Module = m;
}

export function updateWireValues(values) {
    state.wireValues = values;
}

export function updateWireExpressions(expressions) {
    state.wireExpressions = expressions;
}

export function setHighlightedWire(name) {
    state.highlightedWireName = name;
}

export function setInputValue(name, value) {
    state.inputValues[name] = value;
}