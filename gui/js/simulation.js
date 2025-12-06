import { state, setInputValue as updateStateInputValue } from './state.js';
import { drawCircuitDiagram } from './diagram.js';
import { setVerilogData } from './interactions.js';

function escapeHtml(text) {
    const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
    return text.replace(/[&<>"']/g, m => map[m]);
}

export async function analyzeVerilog() {
    const verilogCode = document.getElementById('verilogInput').value;
    const outputDiv = document.getElementById('output');

    if (!verilogCode.trim()) {
        outputDiv.innerHTML = '<span class="error">❌ Error: Please enter Verilog code</span>';
        return;
    }

    try {
        outputDiv.innerHTML = '<div class="loading"><span class="spinner"></span> Analyzing...</div>';

        if (!state.Module || !state.Module.generateNetlistJson) {
            outputDiv.innerHTML = '<span class="error">❌ Error: Wasm module not yet loaded. Please refresh and try again.</span>';
            console.error('Module not ready:', state.Module);
            return;
        }

        let resultJson;
        try {
            resultJson = state.Module.generateNetlistJson(verilogCode);
        } catch (wasmError) {
            console.error('Wasm call error:', wasmError);
            outputDiv.innerHTML = `<span class="error">❌ Wasm Error: ${wasmError.message || wasmError}</span>`;
            return;
        }

        let result;
        try {
            result = JSON.parse(resultJson);
        } catch (parseError) {
            console.error('JSON parse error:', parseError);
            outputDiv.innerHTML = `<span class="error">❌ Parse Error: ${parseError.message}<br>Raw output: ${resultJson}</span>`;
            return;
        }

        if (result.error) {
            outputDiv.innerHTML = `<span class="error">❌ Parsing Error:<br><br>${escapeHtml(result.error)}</span>`;
        } else if (!result.netlist) {
            outputDiv.innerHTML = `<span class="error">❌ Error: No netlist generated<br><br>Response: ${JSON.stringify(result, null, 2)}</span>`;
        } else {
            outputDiv.innerHTML = `<span class="success">✓ Analysis Complete</span>\n\n<pre>${JSON.stringify(result, null, 2)}</pre>`;
            if (result.netlist && result.netlist.length > 0) {
                displayNetlistTable(result.netlist);
            } else {
                console.warn('Empty netlist generated');
            }
        }
    } catch (error) {
        console.error('Unexpected error:', error);
        outputDiv.innerHTML = `<span class="error">❌ Unexpected Error:<br><br>${escapeHtml(error.message || String(error))}<br><br><small>Check console for details</small></span>`;
    }
}

export function displayNetlistTable(netlist) {
    const container = document.getElementById('netlsitContainer');
    let html = '<table class="netlist-table"><thead><tr><th>Output</th><th>Type</th><th>Inputs</th></tr></thead><tbody>';

    netlist.forEach(component => {
        const inputs = component.inputs.join(', ') || 'N/A';
        html += `<tr><td>${component.output}</td><td>${component.type}</td><td>${inputs}</td></tr>`;
    });

    html += '</tbody></table>';
    container.innerHTML = html;

    drawCircuitDiagram(netlist);
    generateInputFields(netlist);
}

export function generateInputFields(netlist) {
    const container = document.getElementById('inputsContainer');
    const inputs = new Set();

    netlist.forEach(component => {
        component.inputs.forEach(input => {
            inputs.add(input);
        });
    });

    netlist.forEach(component => {
        inputs.delete(component.output);
    });

    if (inputs.size === 0) {
        container.innerHTML = '<p style="color: #666;">No inputs found in this circuit.</p>';
        document.getElementById('simulateBtn').style.display = 'none';
        return;
    }

    let html = '<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px;">';

    inputs.forEach(input => {
        html += `
            <div style="border: 1px solid #ddd; padding: 10px; border-radius: 4px;">
                <label style="font-weight: bold; display: block; margin-bottom: 8px;">${input}</label>
                <div style="display: flex; gap: 10px;">
                    <button onclick="setInputValue('${input}', 0)" style="flex: 1; padding: 8px; background: #f44336; color: white; border: none; border-radius: 3px; cursor: pointer;">0</button>
                    <button onclick="setInputValue('${input}', 1)" style="flex: 1; padding: 8px; background: #4CAF50; color: white; border: none; border-radius: 3px; cursor: pointer;">1</button>
                </div>
                <div id="value_${input}" style="margin-top: 8px; text-align: center; padding: 5px; background: #f9f9f9; border-radius: 3px; font-weight: bold; color: #333;">?</div>
            </div>
        `;
    });

    html += '</div>';
    container.innerHTML = html;
    document.getElementById('simulateBtn').style.display = 'block';
}

export function setInputValue(inputName, value) {
    updateStateInputValue(inputName, value);
    document.getElementById('value_' + inputName).textContent = value;
    runSimulation();
}

export function runSimulation() {
    if (!state.Module || !state.Module.simulateCircuit) {
        console.error('Wasm module not ready');
        return;
    }

    const verilogCode = document.getElementById('verilogInput').value;
    const valuesContainer = document.getElementById('valuesContainer');

    try {
        const resultJson = state.Module.simulateCircuit(verilogCode, JSON.stringify(state.inputValues));
        const result = JSON.parse(resultJson);

        if (result.error) {
            valuesContainer.innerHTML = `<span class="error">❌ ${escapeHtml(result.error)}</span>`;
        } else {
            let html = '<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px;">';
            for (const [wire, value] of Object.entries(result.values)) {
                const valueDisplay = value ? '1 ✓' : '0 ✗';
                const color = value ? '#4CAF50' : '#f44336';
                html += `
                    <div style="border: 2px solid ${color}; padding: 12px; border-radius: 4px; text-align: center;">
                        <div style="font-weight: bold; margin-bottom: 8px; color: #333;">${wire}</div>
                        <div style="font-size: 20px; font-weight: bold; color: ${color};">${valueDisplay}</div>
                    </div>
                `;
            }
            html += '</div>';
            valuesContainer.innerHTML = html;

            const dummyExpressions = {
                "a": "INPUT (Pin a)", "b": "INPUT (Pin b)", "c": "INPUT (Pin c)", "d": "INPUT (Pin d)",
                "ab_and": "a & b", "cd_or": "c | d", "not_d": "~d", "y_xor": "a ^ c",
                "y_and": "ab_and", "y_or": "cd_or", "y_mix": "ab_and ^ cd_or"
            };

            const fullData = {
                values: result.values,
                expressions: dummyExpressions
            };

            setVerilogData(fullData);
        }
    } catch (error) {
        console.error('Simulation error:', error);
        valuesContainer.innerHTML = `<span class="error">❌ ${escapeHtml(error.message)}</span>`;
    }
}