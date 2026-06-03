#pragma once

#include <string>
#include <vector>

#include "parser.h"

struct Point
{
    double x = 0.0;
    double y = 0.0;
};

struct WireSegment
{
    Point start;
    Point end;
};

struct Junction
{
    Point location;
};

struct NetLabel
{
    std::string name;
    Point location;
};

struct Component
{
    std::string reference;
    std::string value;

    Point location;

    double rotation = 0.0;
};

struct Schematic
{
    std::vector<WireSegment> wires;
    std::vector<Junction> junctions;
    std::vector<NetLabel> labels;
    std::vector<Component> components;
};

class Interpreter
{
private:

    const std::vector<ASTNode>& pool;

    Schematic schematic;

    void Visit(uint32_t idx);

    bool NodeNameEquals(
        uint32_t idx,
        const char* expected);

    uint32_t FindNamedChild(
        uint32_t idx,
        const char* name);

    double GetNumber(
        uint32_t idx);

    std::string GetText(
        uint32_t idx);

    bool ExtractXY(
        uint32_t xy_node,
        Point& point);

    void ExtractWire(
        uint32_t idx);

    void ExtractJunction(
        uint32_t idx);

public:

    explicit Interpreter(
        const std::vector<ASTNode>& p);

    Schematic Execute(
        uint32_t root);
};