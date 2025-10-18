// ItemRegistry.h
#pragma once
#include "Items_Medicine.h"
#include "Items_Food.h"
#include "Items_Armor.h"
#include "Items_Helmet.h"
#include "Items_Sword.h"
#include "Items_Spear.h"
#include "Items_Axe.h"
#include "Items_Cane.h"
#include "Items_Bow.h"
#include "Items_Gun.h"
#include "Items_Belt.h"
#include "Items_Shoe.h"
#include "Items_Stationery.h"
#include "Items_Ironware.h"
#include "Items_Pottery.h"
#include "Items_FishingTool.h"
#include "Items_Other.h"
#include "Items_Cloak.h"
#include "Items_Costume.h"
#include "Items_Hat.h"
#include "Items_Necklace.h"
#include "Items_Ring.h"
#include "Items_Crystal.h"

// + any others…

struct ItemCategory {
    const wchar_t* name;
    const IconEntry* items;
    size_t count;
};

static const ItemCategory kAllCategories[] = {
    { L"Sword",    kItems_Sword,    kItems_Sword_Count    },
    { L"Spear",    kItems_Spear,    kItems_Spear_Count    },
    { L"Axe",      kItems_Axe,      kItems_Axe_Count      },
    { L"Bow",      kItems_Bow,      kItems_Bow_Count      },
    { L"Gun",      kItems_Gun,      kItems_Gun_Count      },
    { L"Cane",     kItems_Cane,     kItems_Cane_Count     },
    { L"Helmet",   kItems_Helmet,   kItems_Helmet_Count   },
    { L"Armor",    kItems_Armor,    kItems_Armor_Count    },
    { L"Belt",     kItems_Belt,     kItems_Belt_Count     },
    { L"Shoes",    kItems_Shoe,    kItems_Shoe_Count      },
    { L"Food",     kItems_Food,     kItems_Food_Count     },
    { L"Medicine", kItems_Medicine, kItems_Medicine_Count },
    { L"Crystal", kItems_Crystal, kItems_Crystal_Count},
    { L"Stationery", kItems_Stationery, kItems_Stationery_Count },
	{ L"Ironware", kItems_Ironware, kItems_Ironware_Count },
    { L"Pottery", kItems_Pottery, kItems_Pottery_Count },
    { L"Fishing Tool", kItems_FishingTool, kItems_FishingTool_Count },
    { L"Ammunition", kItems_Ammunition, kItems_Ammunition_Count},
    { L"Made Cloak", kItems_Cloak, kItems_Cloak_Count},
    { L"Made Costume", kItems_Costume, kItems_Costume_Count},
    { L"Made Hat", kItems_Hat, kItems_Hat_Count},
    { L"Made Necklace", kItems_Necklace, kItems_Necklace_Count},
    { L"Made Ring", kItems_Ring, kItems_Ring_Count}
};

static constexpr size_t kAllCategoriesCount = _countof(kAllCategories);