import VerilogSimModule from '../VerilogSimWasm.js'; // שים לב לנתיב, הוא יוצא תיקייה אחת למעלה
import { setModule } from './state.js';
import { analyzeVerilog, runSimulation, setInputValue, displayNetlistTable, generateInputFields } from './simulation.js';
import { drawCircuitDiagram } from './diagram.js';
import { handleWireClick, setVerilogData } from './interactions.js';

// Expose functions to global scope so HTML onclick attributes work
window.analyzeVerilog = analyzeVerilog;
window.runSimulation = runSimulation;
window.setInputValue = setInputValue;
window.displayNetlistTable = displayNetlistTable;
window.generateInputFields = generateInputFields;
window.drawCircuitDiagram = drawCircuitDiagram;
window.handleWireClick = handleWireClick;
window.setVerilogData = setVerilogData;

// Initialize Wasm Module
VerilogSimModule().then(m => {
    setModule(m);
    console.log('✓ Wasm module loaded successfully');
    document.getElementById('output').innerHTML = '<span class="success">✓ Wasm module ready</span>';

    const cuteGatesCode = `module cute_gates (
    input wire a,
    input wire b,
    input wire c,
    input wire d,
    output wire y_and,
    output wire y_or,
    output wire y_xor,
    output wire y_mix
);
    wire ab_and;
    wire cd_or;
    wire not_d;
    assign ab_and = a & b;
    assign cd_or = c | d;
    assign y_xor = a ^ c;
    assign not_d = ~d;
    assign y_and = ab_and;
    assign y_or = cd_or;
    assign y_mix = ab_and ^ cd_or;
endmodule`;

    document.getElementById('verilogInput').value = cuteGatesCode;

    // Auto-analyze
    setTimeout(() => {
        window.analyzeVerilog();
    }, 500);
}).catch(err => {
    console.error('✗ Failed to load Wasm module:', err);
    document.getElementById('output').innerHTML = '<span class="error">✗ Failed to load Wasm module: ' + err.message + '</span>';
});

// Key bindings
document.addEventListener('DOMContentLoaded', function () {
    const input = document.getElementById('verilogInput');
    if (input) {
        input.addEventListener('keydown', function (e) {
            if (e.ctrlKey && e.key === 'Enter') {
                window.analyzeVerilog();
            }
        });
    }
});