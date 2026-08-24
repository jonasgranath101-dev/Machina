/**
 * G-Code File Extraction & Translation Utility in C++
 *
 * Features:
 * - Reads input G-Code files line-by-line using std::ifstream to process large files with low memory footprint.
 * - Writes translated coordinates into an output G-Code file stream.
 * - Supports CLI command-line options for custom input/output paths and offset coordinates.
 * - Translates X, Y, Z coordinates and circular arc vectors I, J, K.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <algorithm>

struct Vector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

class GCodeTranslator {
private:
    Vector3D offset_;
    int precision_{4};

    bool isCoordinateAxis(char c) const {
        c = std::toupper(c);
        return (c == 'X' || c == 'Y' || c == 'Z' || c == 'I' || c == 'J' || c == 'K');
    }

    double getOffsetForAxis(char c) const {
        switch (std::toupper(c)) {
            case 'X': case 'I': return offset_.x;
            case 'Y': case 'J': return offset_.y;
            case 'Z': case 'K': return offset_.z;
            default: return 0.0;
        }
    }

public:
    GCodeTranslator(double dx, double dy, double dz, int precision = 4)
        : offset_{dx, dy, dz}, precision_{precision} {}

    /**
     * Translates a single line of G-Code.
     */
    std::string translateLine(const std::string& line) const {
        std::ostringstream result;
        size_t i = 0;
        size_t length = line.length();

        while (i < length) {
            // Preserve full-line comments beginning with ';'
            if (line[i] == ';') {
                result << line.substr(i);
                break;
            }

            // Preserve block comments enclosed in parentheses (...)
            if (line[i] == '(') {
                size_t closingParen = line.find(')', i);
                if (closingParen != std::string::npos) {
                    result << line.substr(i, closingParen - i + 1);
                    i = closingParen + 1;
                    continue;
                }
            }

            // Process targeted coordinate axis words
            if (std::isalpha(line[i]) && isCoordinateAxis(line[i])) {
                char axisLetter = line[i];
                size_t numStart = i + 1;

                if (numStart < length && (std::isdigit(line[numStart]) || line[numStart] == '-' || line[numStart] == '+' || line[numStart] == '.')) {
                    size_t numEnd = numStart;
                    while (numEnd < length && (std::isdigit(line[numEnd]) || line[numEnd] == '.' || line[numEnd] == '-' || line[numEnd] == '+')) {
                        if ((line[numEnd] == '-' || line[numEnd] == '+') && numEnd > numStart) {
                            break;
                        }
                        numEnd++;
                    }

                    std::string numStr = line.substr(numStart, numEnd - numStart);
                    try {
                        double value = std::stod(numStr);
                        double translatedValue = value + getOffsetForAxis(axisLetter);

                        std::ostringstream valStream;
                        valStream << std::fixed << std::setprecision(precision_) << translatedValue;
                        
                        std::string formattedVal = valStream.str();
                        formattedVal.erase(formattedVal.find_last_not_of('0') + 1, std::string::npos);
                        if (formattedVal.back() == '.') {
                            formattedVal.pop_back();
                        }

                        result << axisLetter << formattedVal;
                        i = numEnd;
                        continue;
                    } catch (...) {
                        // Keep unchanged if parsing fails
                    }
                }
            }

            result << line[i];
            i++;
        }

        return result.str();
    }

    /**
     * Extracts lines from input file, translates coordinates, and writes to target destination file.
     */
    bool processFile(const std::string& inputPath, const std::string& outputPath) const {
        std::ifstream inFile(inputPath);
        if (!inFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << inputPath << std::endl;
            return false;
        }

        std::ofstream outFile(outputPath);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file: " << outputPath << std::endl;
            return false;
        }

        std::string line;
        size_t lineCount = 0;
        
        // Line-by-line file extraction stream loop
        while (std::getline(inFile, line)) {
            outFile << translateLine(line) << "\n";
            lineCount++;
        }

        std::cout << "Successfully processed " << lineCount << " lines from '" 
                  << inputPath << "' into '" << outputPath << "'." << std::endl;
        return true;
    }
};

int main(int argc, char* argv[]) {
    std::string inputFile = "input.gcode";
    std::string outputFile = "output.gcode";
    double dx = 100.0, dy = 50.0, dz = -5.0;

    // Check command-line arguments: <input_file> <output_file> [dX] [dY] [dZ]
    if (argc >= 3) {
        inputFile = argv[1];
        outputFile = argv[2];
    }
    if (argc >= 6) {
        dx = std::stod(argv[3]);
        dy = std::stod(argv[4]);
        dz = std::stod(argv[5]);
    }

    // Helper demo: create a dummy input file if one doesn't exist
    {
        std::ifstream checkExists(inputFile);
        if (!checkExists.good()) {
            std::ofstream sampleWriter(inputFile);
            sampleWriter << "O1001 (Sample G-Code File Output)\n"
                         << "G21 (Metric Units)\n"
                         << "G90 (Absolute Positioning)\n"
                         << "G00 X10.0 Y20.0 Z5.0 ; Rapid Move to Start\n"
                         << "G01 Z-2.5 F150.0\n"
                         << "G01 X50.5 Y20.0 F500.0\n"
                         << "G02 X60.0 Y30.0 I0.0 J10.0 ; Clockwise Arc\n"
                         << "G00 Z10.0\n"
                         << "M30\n";
            std::cout << "Generated dummy input file: '" << inputFile << "'\n";
        }
    }

    std::cout << "Processing file extraction & translation..." << std::endl;
    std::cout << "Offsets -> X: " << dx << ", Y: " << dy << ", Z: " << dz << std::endl;

    GCodeTranslator translator(dx, dy, dz);
    if (!translator.processFile(inputFile, outputFile)) {
        return 1;
    }

    return 0;
}