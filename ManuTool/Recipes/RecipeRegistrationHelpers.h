#pragma once
#include "RecipeModel.h"

using NodeEnum = NodeType;
constexpr auto kMat = NodeEnum::Material;
constexpr auto kSub = NodeEnum::Tool;
constexpr auto kTool = NodeEnum::Recipe;

inline void Set(InputNode& n, NodeEnum t, const std::wstring& nm, float q) {
    n.type = t;
    n.name = nm;
    n.quantity = q;
}

inline InputNode Mat(const std::wstring& name, float qty) { InputNode n{}; Set(n, kMat, name, qty); return n; }
inline InputNode Sub(const std::wstring& name, float qty) { InputNode n{}; Set(n, kSub, name, qty); return n; }
inline InputNode Tool(const std::wstring& name, float qty) { InputNode n{}; Set(n, kTool, name, qty); return n; }

inline void AddRecipe(std::wstring name,
    std::initializer_list<InputNode> inputs) {
    Recipe r{};
    r.name = std::move(name);
    r.inputs.assign(inputs.begin(), inputs.end());
    r.yield = 1.0; // default
    RecipesDB::Add(r);
}

inline void AddRecipe(std::wstring name, double yield,
    std::initializer_list<InputNode> inputs) {
    Recipe r{};
    r.name = std::move(name);
    r.inputs.assign(inputs.begin(), inputs.end());
    r.yield = yield; // e.g., 2 for Sesame Oil, 3 for Bread
    RecipesDB::Add(r);
}