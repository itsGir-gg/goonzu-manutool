#pragma once
#include <string>
#include <string_view>
#include <stack>
#include <vector>
#include <unordered_map>
#include <initializer_list>     // <-- add this

// What an input line represents
enum class NodeType : unsigned char {
    Material,   // base resource; consumed
    Tool,       // required, NOT consumed
    Recipe      // reference to another recipe (sub-assembly)
};

struct InputNode {
    NodeType        type;
    std::wstring    name;     // canonical display name (must match your tree item text)
    int             quantity; // per crafted unit; for Tool, this is the count required (usually 1)
};

// A craftable thing (can also represent a base material with no inputs)
struct Recipe {
    std::wstring        name;
    std::vector<InputNode> inputs; // empty => base material (if you want to list it anyway)
    double yield = 1.0;
};

// Global registry API (kept simple for now)
namespace RecipesDB {
    void RegisterAll();
    const Recipe* Find(std::wstring_view name);
    void Add(const Recipe& r);
    std::vector<std::wstring> AllNames();   // <-- only this for enumeration

    // ---------- Inline helpers (single source of truth) ----------
    inline InputNode Mat(std::wstring n, int q)
    {
        return InputNode{ NodeType::Material, std::move(n), q };
    }
    inline InputNode Tool(std::wstring n, int q)
    {
        return InputNode{ NodeType::Tool, std::move(n), q };
    }
    inline InputNode Sub(std::wstring n, int q)
    {
        return InputNode{ NodeType::Recipe, std::move(n), q };
    }

    inline void AddRecipe(std::wstring name, std::initializer_list<InputNode> inputs)
    {
        Recipe r;
        r.name = std::move(name);
        r.inputs.assign(inputs.begin(), inputs.end());
        r.yield = 1.0;
        Add(r);
    }
    inline void AddRecipe(std::wstring name, double yield, std::initializer_list<InputNode> inputs)
    {
        Recipe r;
        r.name = std::move(name);
        r.inputs.assign(inputs.begin(), inputs.end());
        r.yield = yield;
        Add(r);
    }
} // namespace RecipesDB