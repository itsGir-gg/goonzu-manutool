#pragma once
#include <string>
#include <unordered_map>
#include "RecipeModel.h"

// Aggregated result for a target recipe * quantity*.
struct BOMResult {
    std::unordered_map<std::wstring, int> materials; // consumables
    std::unordered_map<std::wstring, int> tools;     // required (not consumed)
};

// Expand a recipe tree into totals.
// - Accumulates materials linearly
// - Tools are combined so duplicates don't multiply with subassemblies
void BuildBOM(const std::wstring& recipeName, int quantity, BOMResult& out);
