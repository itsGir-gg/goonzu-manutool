#include "RecipeModel.h"
#include <algorithm>
#include <cwctype>
#include "recipes/RecipeModules.h"

namespace {
    struct CaseInsensitiveHash {
        size_t operator()(const std::wstring& s) const noexcept {
            std::wstring k(s);
            for (auto& ch : k) ch = (wchar_t)towlower(ch);
            // A tiny hash combiner
            size_t h = 1469598103934665603ull;
            for (wchar_t c : k) { h ^= (size_t)c; h *= 1099511628211ull; }
            return h;
        }
    };
    struct CaseInsensitiveEq {
        bool operator()(const std::wstring& a, const std::wstring& b) const noexcept {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                if (towlower(a[i]) != towlower(b[i])) return false;
            return true;
        }
    };

    using MapT = std::unordered_map<std::wstring, Recipe, CaseInsensitiveHash, CaseInsensitiveEq>;
    MapT gRecipes;
    bool gRegistered = false;

    // small helper
    void add(std::wstring name, std::initializer_list<InputNode> ins) {
        Recipe r; r.name = std::move(name);
        r.inputs.assign(ins.begin(), ins.end());
        RecipesDB::Add(r);
    }
}

void RecipesDB::Add(const Recipe& r) {
    gRecipes[r.name] = r; // case-insensitive key thanks to custom hash/eq
}

const Recipe* RecipesDB::Find(std::wstring_view name) {
    if (!gRegistered) RegisterAll();
    std::wstring key(name);
    auto it = gRecipes.find(key);
    return (it == gRecipes.end()) ? nullptr : &it->second;
}

std::vector<std::wstring> RecipesDB::AllNames() {
    RegisterAll();
    std::vector<std::wstring> out;
    out.reserve(gRecipes.size());
    for (const auto& kv : gRecipes) out.push_back(kv.first);
    std::sort(out.begin(), out.end(),
        [](const std::wstring& a, const std::wstring& b) {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        });
    return out;
}

// ----- Seed data -------------------------------------------------------------

void RecipesDB::RegisterAll() {
    if (gRegistered) return;
    gRegistered = true;

    // Base Items first, followed by compound items
    recipes::RegisterBaseMaterials();
    recipes::RegisterCompoundMaterials();
    // Then, tools
    recipes::RegisterTools();
    // And lastly, items
    recipes::RegisterIronwareRecipes();
    recipes::RegisterMedicineRecipes();
    recipes::RegisterFoodRecipes();
    recipes::RegisterSwordRecipes();
    recipes::RegisterSpearRecipes();
    recipes::RegisterAxeRecipes();
    recipes::RegisterCaneRecipes();
    recipes::RegisterBowRecipes();
    recipes::RegisterGunRecipes();
    recipes::RegisterArmorRecipes();
    recipes::RegisterHelmetRecipes();
    recipes::RegisterBeltRecipes();
    recipes::RegisterShoeRecipes();
    recipes::RegisterCrystalRecipes();
    recipes::RegisterPotteryRecipes();
    recipes::RegisterFishingToolRecipes();
    recipes::RegisterClothesRecipes();
    recipes::RegisterHatRecipes();
    recipes::RegisterCloakRecipes();
    recipes::RegisterNecklaceRecipes();
    recipes::RegisterRingRecipes();
}