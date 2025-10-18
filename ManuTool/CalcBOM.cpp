#include "CalcBOM.h"
#include <string_view>
#include <stack>

static void add(std::unordered_map<std::wstring, int>& dst,
    const std::wstring& k, int v)
{
    if (v == 0) return;
    auto it = dst.find(k);
    if (it == dst.end()) dst.emplace(k, v);
    else it->second += v;
}

void BuildBOM(const std::wstring& recipeName, int quantity, BOMResult& out)
{
    out.materials.clear();
    out.tools.clear();
    if (quantity <= 0) return;

    // rename for clarity: this is "desired OUTPUT units"
    struct Frame { const Recipe* r; int outQty; };
    std::stack<Frame> st;

    if (auto* r = RecipesDB::Find(recipeName))
        st.push({ r, quantity });

    while (!st.empty()) {
        Frame f = st.top(); st.pop();
        if (!f.r) continue;

        // NEW: how many crafts do we need to reach outQty, given recipe->yield?
        const int y = (f.r->yield > 0) ? f.r->yield : 1;
        const int crafts = (f.outQty + y - 1) / y;   // ceil(outQty / yield)

        for (const auto& in : f.r->inputs) {
            // Inputs are specified per craft, so multiply by crafts (not by outQty)
            const int need = in.quantity * crafts;

            switch (in.type) {
            case NodeType::Material:
                add(out.materials, in.name, need);
                break;
            case NodeType::Tool:
                add(out.tools, in.name, need);
                break;
            case NodeType::Recipe:
                if (const Recipe* sub = RecipesDB::Find(in.name))
                    // We need 'need' OUTPUT units of the sub-recipe.
                    st.push({ sub, need });
                else
                    // Unknown sub treated as raw material.
                    add(out.materials, in.name, need);
                break;
            }
        }
    }

    for (auto& kv : out.tools) kv.second = 1;
}