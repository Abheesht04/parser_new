#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm> // Required for std::min

// Include our high-performance engine headers
#include "lexer.h"
#include "parser.h"
#include "interpreter.h" // Your new interpreter engine

// -----------------------------------------------------------------------------
// Debug Helper: Prints the AST out to the console sequentially
// -----------------------------------------------------------------------------
void PrintTree(const std::vector<ASTNode>& pool, uint32_t idx, int depth = 0)
{
    // Check bounds and NULL marker safely
    if (idx == 0 || idx >= pool.size()) return;

    const ASTNode& node = pool[idx];

    // Indent based on tree depth
    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }

    if (node.type == NodeType::List) {
        std::cout << "(LIST)\n";
    }
    else {
        // We have to explicitly build a string from the TokenView so cout knows the length
        std::string text_str(node.text.start, node.text.length);
        std::cout << text_str << "\n";
    }

    // Standard Left-Child Next-Sibling traversal
    PrintTree(pool, node.first_child, depth + 1);
    PrintTree(pool, node.next_sibling, depth);
}

// -----------------------------------------------------------------------------
// JSON Generator: Converts the flat LCRS array into nested JSON
// -----------------------------------------------------------------------------
std::string ASTNodeToJSON(const std::vector<ASTNode>& pool, uint32_t node_idx, int indent_level = 0)
{
    // Index 0 is our universal NULL node
    if (node_idx == 0 || node_idx >= pool.size()) return "null";

    const ASTNode& node = pool[node_idx];
    std::string indent(indent_level * 2, ' ');
    std::ostringstream ss;

    ss << "{\n";
    ss << indent << "  \"type\": ";

    switch (node.type) {
    case NodeType::List:   ss << "\"List\""; break;
    case NodeType::Symbol: ss << "\"Symbol\""; break;
    case NodeType::Number: ss << "\"Number\""; break;
    case NodeType::String: ss << "\"String\""; break;
    }

    // Safely extract and escape text for String, Symbol, and Number nodes
    if (node.type != NodeType::List && node.text.start != nullptr) {
        std::string text_str(node.text.start, node.text.length);

        // Escape quotes and backslashes for valid JSON
        std::string escaped_text;
        for (char c : text_str) {
            if (c == '"') escaped_text += "\\\"";
            else if (c == '\\') escaped_text += "\\\\";
            else if (c == '\n') escaped_text += "\\n";
            else escaped_text += c;
        }
        ss << ",\n" << indent << "  \"text\": \"" << escaped_text << "\"";
    }

    // Add numeric value if applicable
    if (node.type == NodeType::Number) {
        ss << ",\n" << indent << "  \"value\": " << node.number_value;
    }

    // Recursively step into the first_child, and then chain through all next_siblings!
    if (node.first_child != 0) {
        ss << ",\n" << indent << "  \"children\": [\n";

        uint32_t child_idx = node.first_child;
        bool first = true;

        while (child_idx != 0) {
            if (!first) ss << ",\n";
            // Recurse into the child
            ss << indent << "    " << ASTNodeToJSON(pool, child_idx, indent_level + 2);
            first = false;

            // Advance to the next sibling
            child_idx = pool[child_idx].next_sibling;
        }
        ss << "\n" << indent << "  ]";
    }

    ss << "\n" << indent << "}";
    return ss.str();
}

// -----------------------------------------------------------------------------
// Application Entry Point
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // 1. Get the target file (hardcoded for testing, or passed via command line)
    std::string filepath = "ESP32_Status_Monitor.kicad_sch";
    if (argc > 1) {
        filepath = argv[1];
    }

    // 2. Load the file into memory WITH Sentinel Padding
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filepath << "\n";
        return 1;
    }

    size_t file_size = file.tellg();
    file.seekg(0);

    // ALLOCATE FILE SIZE + 1 FOR THE SENTINEL BYTE
    std::vector<char> source_buffer(file_size + 1);
    file.read(source_buffer.data(), file_size);

    // Drop the explicit End-Of-File sentinel marker
    source_buffer[file_size] = '\0';

    // 3. Initialize the Engine
    Lexer lexer(source_buffer.data());
    Parser parser(lexer);

    // 4. Execute parsing
    uint32_t root_node_idx = parser.Parse();
    const auto& node_pool = parser.GetPool();

    if (root_node_idx == 0) {
        std::cerr << "Parsing failed or file was empty.\n";
        return 1;
    }

    // Generate JSON to verify our Left-Child Next-Sibling structure
    std::string json_output = ASTNodeToJSON(node_pool, root_node_idx);

    // Write to disk
    std::ofstream out_file("ast_output.json");
    if (out_file.is_open()) {
        out_file << json_output;
        out_file.close();
    }

    // -----------------------------------------------------
    // Interpreter
    // -----------------------------------------------------

    Interpreter interpreter(node_pool);
    Schematic schematic = interpreter.Execute(root_node_idx);

    // -----------------------------------------------------
    // Dashboard
    // -----------------------------------------------------

    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "                 KICAD PARSER DASHBOARD\n";
    std::cout << "============================================================\n";

    std::cout << "\nFILE INFORMATION\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Path        : " << filepath << "\n";
    std::cout << "File Size   : " << file_size << " bytes\n";

    std::cout << "\nPARSER STATISTICS\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "AST Nodes   : " << node_pool.size() << "\n";
    std::cout << "Root Node   : " << root_node_idx << "\n";

    std::cout << "\nINTERPRETER STATISTICS\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Wires       : " << schematic.wires.size() << "\n";
    std::cout << "Junctions   : " << schematic.junctions.size() << "\n";
    std::cout << "Labels      : " << schematic.labels.size() << "\n";
    std::cout << "Components  : " << schematic.components.size() << "\n";

    std::cout << "\nWIRE PREVIEW\n";
    std::cout << "------------------------------------------------------------\n";

    size_t preview_count = std::min<size_t>(schematic.wires.size(), 10);

    for (size_t i = 0; i < preview_count; i++)
    {
        const auto& wire = schematic.wires[i];

        std::cout
            << "[" << i << "] "
            << "("
            << wire.start.x
            << ", "
            << wire.start.y
            << ") -> ("
            << wire.end.x
            << ", "
            << wire.end.y
            << ")\n";
    }

    std::cout << "\nJUNCTION PREVIEW\n";
    std::cout << "------------------------------------------------------------\n";

    preview_count = std::min<size_t>(schematic.junctions.size(), 10);

    for (size_t i = 0; i < preview_count; i++)
    {
        const auto& junction = schematic.junctions[i];

        std::cout
            << "[" << i << "] "
            << "("
            << junction.location.x
            << ", "
            << junction.location.y
            << ")\n";
    }

    std::cout << "\nOUTPUT FILES\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "ast_output.json\n";
    // Once implemented, you can write interpreter_output.json too
    // std::cout << "interpreter_output.json\n";

    std::cout << "\n============================================================\n";
    std::cout << "                  EXECUTION COMPLETE\n";
    std::cout << "============================================================\n";

    return 0;
}