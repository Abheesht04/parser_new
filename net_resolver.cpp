#include "net_resolver.h"

// ----------------------------------------------------
// Constructor
// ----------------------------------------------------

NetResolver::NetResolver(
    const Schematic& sch)
    : schematic(sch)
{
}

// ----------------------------------------------------
// Build Nodes
// ----------------------------------------------------

void NetResolver::BuildNodes()
{
    nodes.clear();

    uint32_t next_id = 0;

    for (const auto& wire : schematic.wires)
    {
        ConnectivityNode start;

        start.id = next_id++;
        start.position = wire.start;

        nodes.push_back(start);

        ConnectivityNode end;

        end.id = next_id++;
        end.position = wire.end;

        nodes.push_back(end);
    }
}

// ----------------------------------------------------
// Build Edges
// ----------------------------------------------------

void NetResolver::BuildEdges()
{
    edges.clear();

    uint32_t node_index = 0;

    for (const auto& wire : schematic.wires)
    {
        if (node_index + 1 >= nodes.size())
            break;

        Edge edge;

        edge.start_node = node_index;
        edge.end_node = node_index + 1;

        edges.push_back(edge);

        node_index += 2;
    }
}

// ----------------------------------------------------
// Print Nodes
// ----------------------------------------------------

void NetResolver::PrintNodes() const
{
    std::cout
        << "\n===================================\n";

    std::cout
        << "CONNECTIVITY NODES\n";

    std::cout
        << "===================================\n";

    for (const auto& node : nodes)
    {
        std::cout
            << "Node "
            << node.id
            << " -> ("
            << node.position.x
            << ", "
            << node.position.y
            << ")\n";
    }
}

// ----------------------------------------------------
// Print Edges
// ----------------------------------------------------

void NetResolver::PrintEdges() const
{
    std::cout
        << "\n===================================\n";

    std::cout
        << "CONNECTIVITY EDGES\n";

    std::cout
        << "===================================\n";

    for (size_t i = 0;
        i < edges.size();
        i++)
    {
        const Edge& edge =
            edges[i];

        std::cout
            << "Edge "
            << i
            << " : Node "
            << edge.start_node
            << " <--> Node "
            << edge.end_node
            << "\n";
    }
}

// ----------------------------------------------------
// Getters
// ----------------------------------------------------

const std::vector<ConnectivityNode>&
NetResolver::GetNodes() const
{
    return nodes;
}

const std::vector<Edge>&
NetResolver::GetEdges() const
{
    return edges;
}