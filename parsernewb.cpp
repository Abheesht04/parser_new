#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>

#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "net_resolver.h"


void ExportNetResolverJSON(
    const NetResolver& resolver)
{
    std::ofstream file(
        "net_resolver_output.json");

    if (!file.is_open())
        return;

    file << "{\n";
    file << "  \"nodes\": [\n";

    const auto& nodes = resolver.GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        file << "    {\n";
        file << "      \"id\": " << nodes[i].id << ",\n";
        file << "      \"position\": { \"x\": " << nodes[i].position.x << ", \"y\": " << nodes[i].position.y << " }\n";
        file << "    }" << (i + 1 < nodes.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    file << "  \"edges\": [\n";
    const auto& edges = resolver.GetEdges();
    for (size_t i = 0; i < edges.size(); ++i)
    {
        file << "    {\n";
        file << "      \"id\": " << i << ",\n";
        file << "      \"start_node\": " << edges[i].start_node << ",\n";
        file << "      \"end_node\": " << edges[i].end_node << "\n";
        file << "    }" << (i + 1 < edges.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";
}


void ExportInterpreterJSON(
    const Schematic& schematic)
{
    std::ofstream file(
        "interpreter_output.json");

    if (!file.is_open())
        return;

    file << "{\n";

    // Wires
    file << "  \"wires\": [\n";
    for (size_t i = 0; i < schematic.wires.size(); ++i)
    {
        const auto& wire = schematic.wires[i];
        file << "    {\n";
        file << "      \"start\": { \"x\": " << wire.start.x << ", \"y\": " << wire.start.y << " },\n";
        file << "      \"end\": { \"x\": " << wire.end.x << ", \"y\": " << wire.end.y << " }\n";
        file << "    }" << (i + 1 < schematic.wires.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Junctions
    file << "  \"junctions\": [\n";
    for (size_t i = 0; i < schematic.junctions.size(); ++i)
    {
        const auto& junc = schematic.junctions[i];
        file << "    {\n";
        file << "      \"location\": { \"x\": " << junc.location.x << ", \"y\": " << junc.location.y << " }\n";
        file << "    }" << (i + 1 < schematic.junctions.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Labels
    file << "  \"labels\": [\n";
    for (size_t i = 0; i < schematic.labels.size(); ++i)
    {
        const auto& label = schematic.labels[i];
        file << "    {\n";
        file << "      \"name\": \"" << label.name << "\",\n";
        file << "      \"location\": { \"x\": " << label.location.x << ", \"y\": " << label.location.y << " }\n";
        file << "    }" << (i + 1 < schematic.labels.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Components
    file << "  \"components\": [\n";
    for (size_t i = 0; i < schematic.components.size(); ++i)
    {
        const auto& comp = schematic.components[i];
        file << "    {\n";
        file << "      \"reference\": \"" << comp.reference << "\",\n";
        file << "      \"value\": \"" << comp.value << "\",\n";
        file << "      \"location\": { \"x\": " << comp.location.x << ", \"y\": " << comp.location.y << " },\n";
        file << "      \"rotation\": " << comp.rotation << "\n";
        file << "    }" << (i + 1 < schematic.components.size() ? "," : "") << "\n";
    }
    file << "  ]\n";

    file << "}\n";
}


std::string ASTNodeToJSON(
    const std::vector<ASTNode>& pool,
    uint32_t node_idx,
    int indent_level = 0)
{
    if (node_idx == 0 ||
        node_idx >= pool.size())
    {
        return "null";
    }

    const ASTNode& node =
        pool[node_idx];

    std::string indent(
        indent_level * 2,
        ' ');

    std::ostringstream ss;

    ss << "{\n";

    ss << indent
        << "  \"type\": ";

    switch (node.type)
    {
    case NodeType::List:
        ss << "\"List\"";
        break;

    case NodeType::Symbol:
        ss << "\"Symbol\"";
        break;

    case NodeType::Number:
        ss << "\"Number\"";
        break;

    case NodeType::String:
        ss << "\"String\"";
        break;
    }

    if (node.type != NodeType::List &&
        node.text.start != nullptr)
    {
        std::string text(
            node.text.start,
            node.text.length);

        ss << ",\n"
            << indent
            << "  \"text\": \""
            << text
            << "\"";
    }

    if (node.type == NodeType::Number)
    {
        ss << ",\n"
            << indent
            << "  \"value\": "
            << node.number_value;
    }

    if (node.first_child != 0)
    {
        ss << ",\n"
            << indent
            << "  \"children\": [\n";

        uint32_t child =
            node.first_child;

        bool first = true;

        while (child != 0)
        {
            if (!first)
            {
                ss << ",\n";
            }

            ss << indent
                << "    "
                << ASTNodeToJSON(
                    pool,
                    child,
                    indent_level + 2);

            first = false;

            child =
                pool[child].next_sibling;
        }

        ss << "\n"
            << indent
            << "  ]";
    }

    ss << "\n"
        << indent
        << "}";

    return ss.str();
}




int main(int argc, char** argv)
{
    // ---------------------------------------------------------
    // File Selection
    // ---------------------------------------------------------

    std::string filepath =
        "ESP32_Status_Monitor.kicad_sch";

    if (argc > 1)
    {
        filepath = argv[1];
    }

    // ---------------------------------------------------------
    // Load File
    // ---------------------------------------------------------

    std::ifstream file(
        filepath,
        std::ios::binary |
        std::ios::ate);

    if (!file.is_open())
    {
        std::cerr
            << "\nERROR: Could not open file:\n"
            << filepath
            << "\n";

        return 1;
    }

    size_t file_size =
        static_cast<size_t>(
            file.tellg());

    file.seekg(0);

    std::vector<char> source_buffer(
        file_size + 1);

    file.read(
        source_buffer.data(),
        file_size);

    source_buffer[file_size] = '\0';

    file.close();

    // ---------------------------------------------------------
    // Lexer + Parser
    // ---------------------------------------------------------

    auto parse_start =
        std::chrono::high_resolution_clock::now();

    Lexer lexer(
        source_buffer.data());

    Parser parser(
        lexer);

    uint32_t root_node_idx =
        parser.Parse();

    auto parse_end =
        std::chrono::high_resolution_clock::now();

    double parse_time_ms =
        std::chrono::duration<double,
        std::milli>(
            parse_end -
            parse_start).count();

    if (root_node_idx == 0)
    {
        std::cerr
            << "\nERROR: Parsing failed.\n";

        return 1;
    }

    const auto& node_pool =
        parser.GetPool();

    // ---------------------------------------------------------
    // AST JSON
    // ---------------------------------------------------------

    std::string ast_json =
        ASTNodeToJSON(
            node_pool,
            root_node_idx);

    {
        std::ofstream out(
            "ast_output.json");

        if (out.is_open())
        {
            out << ast_json;
        }
    }

    // ---------------------------------------------------------
    // Interpreter
    // ---------------------------------------------------------

    Interpreter interpreter(
        node_pool);

    Schematic schematic =
        interpreter.Execute(
            root_node_idx);

    ExportInterpreterJSON(schematic);

    // ---------------------------------------------------------
    // Net Resolver
    // ---------------------------------------------------------

    NetResolver resolver(
        schematic);

    resolver.BuildNodes();

    resolver.BuildEdges();

    ExportNetResolverJSON(resolver);

    // ---------------------------------------------------------
    // Terminal Report
    // ---------------------------------------------------------

    std::cout
        << "\n========================================================\n";

    std::cout
        << "                KICAD PARSER TEST HARNESS\n";

    std::cout
        << "========================================================\n";

    std::cout
        << "\nFILE\n";

    std::cout
        << "--------------------------------------------------------\n";

    std::cout
        << "Path            : "
        << filepath
        << "\n";

    std::cout
        << "Size            : "
        << file_size
        << " bytes\n";

    std::cout
        << "\nPARSER\n";

    std::cout
        << "--------------------------------------------------------\n";

    std::cout
        << "Parse Time      : "
        << parse_time_ms
        << " ms\n";

    std::cout
        << "AST Nodes       : "
        << node_pool.size()
        << "\n";

    std::cout
        << "Root Node       : "
        << root_node_idx
        << "\n";

    std::cout
        << "\nINTERPRETER\n";

    std::cout
        << "--------------------------------------------------------\n";

    std::cout
        << "Wires           : "
        << schematic.wires.size()
        << "\n";

    std::cout
        << "Junctions       : "
        << schematic.junctions.size()
        << "\n";

    std::cout
        << "Labels          : "
        << schematic.labels.size()
        << "\n";

    std::cout
        << "Components      : "
        << schematic.components.size()
        << "\n";

    std::cout
        << "\nNET RESOLVER\n";

    std::cout
        << "--------------------------------------------------------\n";

    std::cout
        << "Connectivity Nodes : "
        << resolver.GetNodes().size()
        << "\n";

    std::cout
        << "Connectivity Edges : "
        << resolver.GetEdges().size()
        << "\n";

    std::cout
        << "\nFILES GENERATED\n";

    std::cout
        << "--------------------------------------------------------\n";

    std::cout
        << "ast_output.json\n"
        << "interpreter_output.json\n"
        << "net_resolver_output.json\n";

    std::cout
        << "\n========================================================\n";

    if (schematic.wires.empty())
    {
        std::cout
            << "\nWARNING: No wires extracted.\n";
    }

    if (schematic.junctions.empty())
    {
        std::cout
            << "\nWARNING: No junctions extracted.\n";
    }

    return 0;
}