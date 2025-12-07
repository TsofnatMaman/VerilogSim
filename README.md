# Verilog Visualization Simulator

MyVerilogSim is a lightweight educational simulator for the Verilog Hardware Description Language (HDL).
It provides a minimal yet functional environment to parse and execute simple Verilog modules,
with the goal of helping students and developers understand the basics of hardware simulation
without the complexity of full-featured tools.

## ✨ Features

  * Simple and clean design focused on core Verilog concepts
  * Execution of basic Verilog constructs and logic
  * **Powered by WebAssembly (WASM):** Core simulation logic is compiled to WASM for efficient, near-native execution speed directly within the browser.
  * **Visual Circuit Diagram Generation:** The tool automatically draws a simplified, interactive logic gate diagram based on the parsed netlist, helping visualize the structure of the digital circuit.
  * Intended as a learning tool or a foundation for more advanced simulators

-----

## 🎯 Goals

This project is not a replacement for professional simulators (like Verilator or ModelSim),
but rather a minimal framework to demonstrate how a Verilog simulation engine works internally.

-----

## 🚀 Getting Started

To run MyVerilogSim locally, follow these simple steps:

### 1\. Navigate to the GUI Directory

You must start the server from the directory containing the HTML and application files:

```bash
cd gui
```

### 2\. Start a Local Web Server

  * Open your terminal or command prompt inside the **`GUI`** directory.
  * Run the following Python command to start a simple HTTP server on port 8080:
    ```bash
    python -m http.server 8080
    ```

### 3\. Access the Application

  * Open your web browser.
  * Navigate to the local server address:
    ```
    http://localhost:8080
    ```

The application will load in your browser, allowing you to input Verilog code and view the simulation results alongside the **visual circuit diagram**.
