![Logo Description](/Icon.png)

## Bash

```

#### ** C++ execution **

g++ -std=c++17 gcode_translator_file.cpp -o gcode_translator_cpp
./gcode_translator_cpp input.gcode output.gcode 100.0 50.0 -5.0

#### ** Rust Execution **

rustc gcode_translator_file.rs -o gcode_translator_rs
./gcode_translator_rs input.gcode output.gcode 50.0 50.0 -2.0

```

```

#### ** Run Example **

O1002 (Rust File Extraction G-Code Sample)
G21 (Metric Units)
G90 (Absolute Positioning)
G00 X15.0 Y25.0 Z10.0 ; Rapid Position
G01 Z-1.0 F200.0
G01 X45.0 Y25.0 F600.0
G03 X55.0 Y35.0 I0.0 J10.0 (Counter-Clockwise Arc)
G00 Z10.0
M30

```

# Machina

A mechatronic g-code block system translator. In C++ and Rust.
