#include "interpreter.h"

#include <cstring>
#include <iostream>

// ---------------------------------------------------------
// Constructor
// ---------------------------------------------------------

Interpreter::Interpreter(
    const std::vector<ASTNode>& p)
    : pool(p)
{
}

// ---------------------------------------------------------
// Execute
// ---------------------------------------------------------

Schematic Interpreter::Execute(
    uint32_t root)
{
    Visit(root);

    return schematic;
}

// ---------------------------------------------------------
// GetText
// ---------------------------------------------------------

std::string Interpreter::GetText(
    uint32_t idx)
{
    if (idx == 0)
        return "";

    const ASTNode& node = pool[idx];

    if (node.text.start == nullptr)
        return "";

    return std::string(
        node.text.start,
        node.text.length);
}

// ---------------------------------------------------------
// GetNumber
// ---------------------------------------------------------

double Interpreter::GetNumber(
    uint32_t idx)
{
    if (idx == 0)
        return 0.0;

    return pool[idx].number_value;
}

// ---------------------------------------------------------
// NodeNameEquals
// ---------------------------------------------------------

bool Interpreter::NodeNameEquals(
    uint32_t idx,
    const char* expected)
{
    if (idx == 0)
        return false;

    const ASTNode& node = pool[idx];

    if (node.type != NodeType::List)
        return false;

    uint32_t first =
        node.first_child;

    if (first == 0)
        return false;

    const ASTNode& name_node =
        pool[first];

    if (name_node.type != NodeType::Symbol)
        return false;

    size_t expected_len =
        std::strlen(expected);

    if (name_node.text.length != expected_len)
        return false;

    return std::memcmp(
        name_node.text.start,
        expected,
        expected_len) == 0;
}

// ---------------------------------------------------------
// FindNamedChild
// ---------------------------------------------------------

uint32_t Interpreter::FindNamedChild(
    uint32_t idx,
    const char* name)
{
    if (idx == 0)
        return 0;

    uint32_t child =
        pool[idx].first_child;

    if (child != 0)
    {
        child =
            pool[child].next_sibling;
    }

    while (child != 0)
    {
        if (NodeNameEquals(child, name))
        {
            return child;
        }

        child =
            pool[child].next_sibling;
    }

    return 0;
}

// ---------------------------------------------------------
// ExtractXY
// Handles:
// (xy 10 20)
// ---------------------------------------------------------

bool Interpreter::ExtractXY(
    uint32_t xy_node,
    Point& point)
{
    if (!NodeNameEquals(xy_node, "xy"))
        return false;

    uint32_t child =
        pool[xy_node].first_child;

    child =
        pool[child].next_sibling;

    if (child == 0)
        return false;

    point.x =
        GetNumber(child);

    child =
        pool[child].next_sibling;

    if (child == 0)
        return false;

    point.y =
        GetNumber(child);

    return true;
}

// ---------------------------------------------------------
// ExtractWire
//
// (wire
//   (pts
//      (xy x1 y1)
//      (xy x2 y2)
//   )
// )
// ---------------------------------------------------------

void Interpreter::ExtractWire(
    uint32_t idx)
{
    uint32_t pts =
        FindNamedChild(idx, "pts");

    if (pts == 0)
        return;

    uint32_t child =
        pool[pts].first_child;

    child =
        pool[child].next_sibling;

    Point p1{};
    Point p2{};

    bool found_first = false;

    while (child != 0)
    {
        if (NodeNameEquals(child, "xy"))
        {
            if (!found_first)
            {
                ExtractXY(child, p1);
                found_first = true;
            }
            else
            {
                ExtractXY(child, p2);

                WireSegment wire;

                wire.start = p1;
                wire.end = p2;

                schematic.wires.push_back(wire);

                std::cout
                    << "Wire: ("
                    << p1.x << ", "
                    << p1.y << ") -> ("
                    << p2.x << ", "
                    << p2.y << ")\n";

                break;
            }
        }

        child =
            pool[child].next_sibling;
    }
}

// ---------------------------------------------------------
// ExtractJunction
//
// (junction
//   (at x y)
// )
// ---------------------------------------------------------

void Interpreter::ExtractJunction(
    uint32_t idx)
{
    uint32_t at =
        FindNamedChild(idx, "at");

    if (at == 0)
        return;

    uint32_t child =
        pool[at].first_child;

    child =
        pool[child].next_sibling;

    if (child == 0)
        return;

    Junction junction;

    junction.location.x =
        GetNumber(child);

    child =
        pool[child].next_sibling;

    if (child == 0)
        return;

    junction.location.y =
        GetNumber(child);

    schematic.junctions.push_back(
        junction);

    std::cout
        << "Junction: ("
        << junction.location.x
        << ", "
        << junction.location.y
        << ")\n";
}

// ---------------------------------------------------------
// Visit
// ---------------------------------------------------------

void Interpreter::Visit(
    uint32_t idx)
{
    if (idx == 0)
        return;

    if (NodeNameEquals(idx, "wire"))
    {
        ExtractWire(idx);
    }

    if (NodeNameEquals(idx, "junction"))
    {
        ExtractJunction(idx);
    }

    uint32_t child =
        pool[idx].first_child;

    while (child != 0)
    {
        Visit(child);

        child =
            pool[child].next_sibling;
    }
}