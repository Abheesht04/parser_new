#pragma once

#include "interpreter.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>

struct ConnectivityNode
{
    Point position;
    uint32_t id = 0;
};

struct Edge
{
    uint32_t start_node = 0;
    uint32_t end_node = 0;
};

class NetResolver
{
private:

    const Schematic& schematic;

    std::vector<ConnectivityNode> nodes;
    std::vector<Edge> edges;

public:

    explicit NetResolver(
        const Schematic& sch);

    void BuildNodes();

    void BuildEdges();

    void PrintNodes() const;

    void PrintEdges() const;

    const std::vector<ConnectivityNode>&
        GetNodes() const;

    const std::vector<Edge>&
        GetEdges() const;
};