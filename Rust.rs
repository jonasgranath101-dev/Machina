//! G-Code File Extraction & Translation Utility in Rust
//!
//! Features:
//! - Streams G-Code line-by-line using `BufReader` and `BufWriter` for memory efficiency.
//! - Extracts G-Code files from disk, performs offset coordinate translations, and saves the output.
//! - Accepts optional CLI arguments: <input_file> <output_file> [dx] [dy] [dz]

use std::env;
use std::fmt::Write as FmtWrite;
use std::fs::File;
use std::io::{self, BufRead, BufReader, BufWriter, Write};
use std::path::Path;

/// Represents 3D offset vectors.
#[derive(Debug, Clone, Copy)]
pub struct Vector3D {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

pub struct GCodeTranslator {
    offset: Vector3D,
    precision: usize,
}

impl GCodeTranslator {
    pub fn new(dx: f64, dy: f64, dz: f64, precision: usize) -> Self {
        Self {
            offset: Vector3D { x: dx, y: dy, z: dz },
            precision,
        }
    }

    fn is_coordinate_axis(c: char) -> bool {
        matches!(c.to_ascii_uppercase(), 'X' | 'Y' | 'Z' | 'I' | 'J' | 'K')
    }

    fn get_offset_for_axis(&self, c: char) -> f64 {
        match c.to_ascii_uppercase() {
            'X' | 'I' => self.offset.x,
            'Y' | 'J' => self.offset.y,
            'Z' | 'K' => self.offset.z,
            _ => 0.0,
        }
    }

    /// Translates a single line of G-Code.
    pub fn translate_line(&self, line: &str) -> String {
        let mut result = String::with_capacity(line.len());
        let chars: Vec<char> = line.chars().collect();
        let len = chars.len();
        let mut i = 0;

        while i < len {
            let ch = chars[i];

            // 1. Preserve full-line comments beginning with ';'
            if ch == ';' {
                result.push_str(&line[i..]);
                break;
            }

            // 2. Preserve inline block comments (...)
            if ch == '(' {
                if let Some(closing_idx) = line[i..].find(')') {
                    let end_pos = i + closing_idx + 1;
                    result.push_str(&line[i..end_pos]);
                    i = end_pos;
                    continue;
                }
            }

            // 3. Translate coordinate words
            if ch.is_ascii_alphabetic() && Self::is_coordinate_axis(ch) {
                let num_start = i + 1;

                if num_start < len
                    && (chars[num_start].is_ascii_digit()
                        || chars[num_start] == '-'
                        || chars[num_start] == '+'
                        || chars[num_start] == '.')
                {
                    let mut num_end = num_start;
                    while num_end < len {
                        let nc = chars[num_end];
                        if nc.is_ascii_digit() || nc == '.' || nc == '-' || nc == '+' {
                            if (nc == '-' || nc == '+') && num_end > num_start {
                                break;
                            }
                            num_end += 1;
                        } else {
                            break;
                        }
                    }

                    let num_str: String = chars[num_start..num_end].iter().collect();

                    if let Ok(val) = num_str.parse::<f64>() {
                        let translated = val + self.get_offset_for_axis(ch);
                        let formatted_num = format!("{:.1$}", translated, self.precision);
                        let trimmed = formatted_num
                            .trim_end_matches('0')
                            .trim_end_matches('.');

                        let _ = write!(result, "{}{}", ch, trimmed);
                        i = num_end;
                        continue;
                    }
                }
            }

            result.push(ch);
            i += 1;
        }

        result
    }

    /// Reads from source input file, translates line by line, and writes out to target file path.
    pub fn process_file(&self, input_path: &str, output_path: &str) -> io::Result<()> {
        let input_file = File::open(input_path)?;
        let reader = BufReader::new(input_file);

        let output_file = File::create(output_path)?;
        let mut writer = BufWriter::new(output_file);

        let mut lines_processed = 0usize;

        for line_result in reader.lines() {
            let line = line_result?;
            let translated_line = self.translate_line(&line);
            writeln!(writer, "{}", translated_line)?;
            lines_processed += 1;
        }

        // Flush remaining buffer data to disk
        writer.flush()?;

        println!(
            "Successfully extracted & translated {} lines from '{}' into '{}'.",
            lines_processed, input_path, output_path
        );
        Ok(())
    }
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();

    let mut input_file = String::from("input.gcode");
    let mut output_file = String::from("output.gcode");
    let mut dx = 50.0;
    let mut dy = 50.0;
    let mut dz = -2.0;

    if args.len() >= 3 {
        input_file = args[1].clone();
        output_file = args[2].clone();
    }
    if args.len() >= 6 {
        dx = args[3].parse().unwrap_or(50.0);
        dy = args[4].parse().unwrap_or(50.0);
        dz = args[5].parse().unwrap_or(-2.0);
    }

    // Helper demo: create input file if missing
    if !Path::new(&input_file).exists() {
        let sample = "\
O1002 (Rust File Extraction G-Code Sample)
G21 (Metric Units)
G90 (Absolute Positioning)
G00 X15.0 Y25.0 Z10.0 ; Rapid Position
G01 Z-1.0 F200.0
G01 X45.0 Y25.0 F600.0
G03 X55.0 Y35.0 I0.0 J10.0 (Counter-Clockwise Arc)
G00 Z10.0
M30";
        std::fs::write(&input_file, sample)?;
        println!("Generated dummy input file: '{}'", input_file);
    }

    println!("Starting G-code file processing...");
    println!("Offsets -> X: {}, Y: {}, Z: {}", dx, dy, dz);

    let translator = GCodeTranslator::new(dx, dy, dz, 3);
    translator.process_file(&input_file, &output_file)?;

    Ok(())
}
