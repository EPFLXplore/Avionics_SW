#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

// Structure to hold generated struct info.
struct GeneratedStruct {
    std::string name;
    std::string code;
};


// Structure to hold a field's type and name.
struct Field {
    std::string type;
    std::string name;
};
std::unordered_map<std::string, std::string> TYPE_MAP = {
    {"uint16", "uint16_t"},
    {"uint8", "uint8_t"},
    {"float32", "float"},
    {"float64", "double"},
    {"bool","string"},
};

// Extract the struct name from the filename (removing the extension).
std::string getStructName(const std::string& filename) {
    return std::filesystem::path(filename).stem().string();
}

void generate_message_file(std::string folderPath, std::string outputFilename){
    // Check if the input directory exists.
    if (!std::filesystem::exists(folderPath)) {
        std::cerr << "Directory " << folderPath << " does not exist." << std::endl;
        return;
    }
    
    // Open the output file at the specified path.
    std::ofstream outfile(outputFilename);
    if (!outfile) {
        std::cerr << "Error creating output file: " << outputFilename << std::endl;
        return;
    }
    
    // Write the header guard and include <iostream>.
    outfile << "#pragma once\n\n";
    outfile << "#include <iostream>\n\n";
    
    // Iterate through all files in the folder.
    for (const auto &entry : std::filesystem::directory_iterator(folderPath)) {
        // Process only regular files with a .msg extension.
        if (entry.is_regular_file() && entry.path().extension() == ".msg") {
            std::ifstream infile(entry.path());
            if (!infile) {
                std::cerr << "Error opening file: " << entry.path() << std::endl;
                continue;
            }
            
            // Derive the struct name from the file name.
            std::string structName = getStructName(entry.path().string());
            std::vector<Field> fields;
            std::string line;
            
            // Read each line and parse the type and variable name,
            // skipping lines that are empty or start with '#'.
            while (std::getline(infile, line)) {
                if (line.empty() || line[0] == '#')
                    continue;
                    
                std::istringstream iss(line);
                std::string type, name;
                if (!(iss >> type >> name)) {
                    std::cerr << "Error parsing line in " << entry.path() << ": " << line << std::endl;
                    continue;
                }
                
                // Add a _t suffix to uint8 and uint16 cases.
                if (type == "uint8") {
                    type = "uint8_t";
                } else if (type == "uint16") {
                    type = "uint16_t";
                }
                
                fields.push_back({type, name});
            }
            infile.close();
            
            // Write the struct definition to the aggregated header file.
            outfile << "struct " << structName << " {\n";
            for (const auto &field : fields) {
                outfile << "    " << field.type << " " << field.name << ";\n";
            }
            outfile << "};\n\n";
            
            std::cout << "Processed file: " << entry.path() << std::endl;
        }
    }
    
    outfile.close();
    std::cout << "Generated aggregated header file: " << outputFilename << std::endl;
}

int main(){
    generate_message_file("lib/ERC_SE_CustomMessages/msg/not_hd", "lib/Packets/packet_definition.hpp");
    return 0;
}