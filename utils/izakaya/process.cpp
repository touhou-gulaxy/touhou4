#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <locale>
#include <vector>
#include <array>
#include <cstdint>
#include <codecvt>
#include <variant>
#include <format>

std::vector<std::string> ingredients_keys;
std::vector<std::string> ingredients_names;
std::vector<std::string> ingredients_descs;
std::vector<std::string> results_keys;
std::vector<std::string> results_names;
std::vector<std::string> foods_descs;

template<typename T>
const char* get_name(T idx, std::vector<std::string> map);

struct recipe
{
    std::array<int_fast8_t, 5> slots = {-1, -1, -1, -1, -1};
    int16_t result = -1;

    const char* result_name()
    {
        return results_names[result].c_str();
    }

    const char* key()
    {
        return results_keys[result].c_str();
    }

    const char* slot_key(int_fast8_t idx)
    {
        if (idx < 0 || idx >= 5)
            return "null";
        auto slot = slots[idx];
        if (slot < 0 || slot >= ingredients_keys.size())
            return "null";
        return ingredients_keys[slot].c_str();
    }
};

struct ingredient
{
    std::vector<int16_t> results;
    int16_t desc_index = -1;
    int_fast8_t id;

    const char* name()
    {
        return ingredients_names[id].c_str();
    }

    const char* desc()
    {
        return ingredients_descs[id].c_str();
    }

    const char* key()
    {
        return ingredients_keys[id].c_str();
    }
};

struct food
{
    std::vector<int_fast8_t> results;
    int16_t desc_index = -1;
    int16_t id;

    const char* name()
    {
        return results_names[id].c_str();
    }

    const char* desc()
    {
        return foods_descs[id].c_str();
    }

    const char* key()
    {
        return results_keys[id].c_str();
    }
};

template<typename T>
T find_matches(const char* name, std::vector<std::string> names)
{
    for (T i = static_cast<T>(0); i < names.size(); ++i)
    {
        if (std::strcmp(name, names[i].c_str()) == 0)
            return i;
    }
    return static_cast<T>(-1);
}

template<typename T>
const char* get_name(T idx, std::vector<std::string> map)
{
    if (idx >= static_cast<T>(0) && idx < static_cast<T>(map.size()))
        return map[idx].c_str();
    return "unknown";
}

void build_maps(const char* key_file, const char* map_file, bool ingredients = true)
{
    std::ifstream ifs;
    if (ingredients)
    {
        ifs.open(key_file);
        char line[127];
        while(!ifs.eof())
        {
            ifs.getline(line, 127);
            if (std::strcmp(line, "") == 0)
                continue;
            ingredients_keys.push_back(line);
        }
        ifs.close();
        ifs.open(map_file);
        while(!ifs.eof())
        {
            ifs.getline(line, 127);
            if (std::strcmp(line, "") == 0)
                continue;
            ingredients_names.push_back(line);
        }
        ifs.close();
    }
    else
    {
        ifs.open(key_file);
        char line[127];
        while(!ifs.eof())
        {
            ifs.getline(line, 127);
            if (std::strcmp(line, "") == 0)
                continue;
            results_keys.push_back(line);
        }
        ifs.close();
        ifs.open(map_file);
        while(!ifs.eof())
        {
            ifs.getline(line, 127);
            if (std::strcmp(line, "") == 0)
                continue;
            results_names.push_back(line);
        }
        ifs.close();
    }
}

#define _IF_CONSTEXPR(INGREDIENTS) if constexpr (INGREDIENTS) \
                                   {
#define _END_IF }
#define _ELSE } \
              else \
              {

template<bool ingredients = true>
std::vector<std::variant<ingredient, food>> build_izakaya_items(const char* desc_file)
{
    using item_type = std::variant<ingredient, food>;
    std::vector<std::variant<ingredient, food>> items;
    std::ifstream ifs;
    ifs.open(desc_file);
    std::string word, tmp;
    size_t i = 0;
    while(!ifs.eof())
    {
        std::string line;
        while(std::getline(ifs, line))
        {
            if(line.empty())
                continue;
            auto pos = line.find(':');
            if(pos == std::string::npos)
                continue;
            auto name = line.substr(0, pos), desc = line.substr(pos + 1);
            desc.erase(std::remove(desc.begin(), desc.end(), '\r'), desc.end());

            int16_t idx;
            _IF_CONSTEXPR(ingredients)
            idx = find_matches<int16_t>(name.c_str(), ingredients_names);
            _ELSE
            idx = find_matches<int16_t>(name.c_str(), results_names);
            _END_IF
            if (idx < 0)
            {
                printf("WARNING: failed to index item: %s(l=%lu), same index result is %s(l=%lu).\n", name.c_str(), name.length(), get_name(i, results_names), std::string(get_name(i++, results_names)).length());
                continue;
            }
            item_type item;
            _IF_CONSTEXPR(ingredients)
            item = ingredient();
            std::get<ingredient>(item).id = idx;
            std::get<ingredient>(item).desc_index = idx;
            ingredients_descs[idx] = desc;
            _ELSE
            item = food();
            std::get<food>(item).id = idx;
            std::get<food>(item).desc_index = idx;
            foods_descs[idx] = desc;
            _END_IF
            items.push_back(item);
            ++i;
        }
    }
    ifs.close();
    return items;
}

#undef _IF_CONSTEXPR
#undef _END_IF
#undef _ELSE

std::vector<recipe> build_recipes(const char* recipe_file)
{
    std::vector<recipe> recipes;
    std::ifstream ifs;
    ifs.open(recipe_file);
    char line[127];
    std::string tmp, word;
    while(!ifs.eof())
    {
        ifs.getline(line, 127);
        if (strcmp(line, "") != 0)
        {
            tmp.clear();
            word = std::string(line);
            //printf("%s\n", word.c_str());
            recipe item;
            size_t slt_idx = 0;
            for (char ch : word)
            {
                if (ch == ' ')
                {
                    auto slot = find_matches<int_fast8_t>(tmp.c_str(), ingredients_names);
                    if (slt_idx < 5)
                        item.slots[slt_idx++] = slot;
                    //printf("find ingredient token: %s, id=%d, key=%s\n", tmp.c_str(), slot,  get_name(slot, ingredients_keys));
                    tmp.clear();
                }
                else if (ch == '-')
                    continue;
                else if (ch == '>')
                {
                    auto slot = find_matches<int_fast8_t>(tmp.c_str(), ingredients_names);
                    if (slt_idx < 5)
                        item.slots[slt_idx++] = slot;
                    // printf("find ingredient token: %s, id=%d, key=%s\n", tmp.c_str(), slot, get_name(slot, ingredients_keys));
                    tmp.clear();
                }
                else if (ch == '\0' || ch == '\n' || ch == '\r')
                {
                    break;
                }
                else
                    tmp += ch;
            }
            auto res_idx = find_matches<int16_t>(tmp.c_str(), results_names);
            // printf("find result token: %s, id=%d, key=%s\n", tmp.c_str(), res_idx, get_name(res_idx, results_keys));
            item.result = res_idx;
            recipes.emplace_back(item);
        }
    }
    ifs.close();
    return recipes;
}

int main()
{
    build_maps("./izakaya_ingredients_names.txt", "izakaya_ingredients_maps.txt");
    build_maps("./izakaya_results_names.txt", "izakaya_results_maps.txt", false);
    ingredients_descs.resize(ingredients_keys.size());
    foods_descs.resize(results_keys.size());

    // izakaya_ingredients_descs.txt
    auto recipes = build_recipes("./recipes.txt");
    auto ingredients = build_izakaya_items("./izakaya_ingredients_descs.txt");
    auto foods = build_izakaya_items<false>("./izakaya_results_descs.txt");
    printf("scaned %lu recipes.\nindexed %lu ingredients.\nindexed %lu foods.\n", recipes.size(), ingredients.size(), foods.size());
    for (auto& item : ingredients)
    {
        auto& item_act = std::get<ingredient>(item);
        // printf("try to find recipes for %s:\n", item_act.key());
        for (auto recipe : recipes)
        {
            auto slot = std::find(recipe.slots.begin(), recipe.slots.end(), item_act.id);
            if (slot != recipe.slots.end())
            {
                item_act.results.emplace_back(recipe.result);
                // printf("  - find a recipe contains the ingredient: %s\n", recipe.key());
            }
        }
    }
    for (auto& item : foods)
    {
        auto& item_food = std::get<food>(item);
        auto recipe = std::find_if(recipes.begin(), recipes.end(), [item_food](const struct recipe r){ return r.result == item_food.id; });
        if (recipe != recipes.end())
        {
            for (auto ingr : (*recipe).slots)
                item_food.results.push_back(ingr);
        }
    }

    std::ofstream loc_file("./izakaya_locs.yml");
    std::format_to(std::ostreambuf_iterator<char>(loc_file), "l_simp_chinese:\n");
    //  building_izakaya_ingredient_tofu: "£izakaya_ingredients_item_tofu£豆腐"
    //  building_izakaya_ingredient_tofu_desc: ""
    //  building_izakaya_ingredient_tofu_tt: "\n食材数量：§Y[owner.izakaya_ingredient_tofu_val]/[get_izakaya_ingredient_max_val]§!"
    static constexpr auto building_loc_name = " building_izakaya_ingredient_{:s}: \"£izakaya_ingredients_item_{:s}£{:s}\"\n";
    static constexpr auto building_loc_desc = " building_izakaya_ingredient_{:s}_desc: \"{:s}\"\n";
    static constexpr auto building_loc_tooltip_prefix = " building_izakaya_ingredient_{:s}_tt: \"关联料理：";
    static constexpr auto building_loc_tooltip_endfix = "\\n食材数量：§Y[owner.izakaya_ingredient_{:s}_val]/[get_izakaya_ingredient_max_val]§!\"\n";
    //  izakaya_food_tsukimi_mochi: "£izakaya_result_foods_item_tsukimi_mochi£月见饼"
    //  izakaya_food_tsukimi_mochi_desc: ""
    static constexpr auto concept_loc_name = " izakaya_food_{:s}: \"£izakaya_result_foods_item_{:s}£{:s}\"\n";
    static constexpr auto concept_loc_desc_prefix = " izakaya_food_{:s}_desc: \"需求：§Y烧烤架§!\\n原材料：";
    static constexpr auto concept_loc_desc_endfix = "\\n{:s}\"\n";
    // inline_script = {
	//     script = recipes/izakaya_barbecue_triggered_name recipe_result = fantasy_craze
	//     slot_1 = potatoes slot_2 = honey slot_3 = null slot_4 = null slot_5 = null
    //  }
    static constexpr auto inline_district_names = "inline_script = {{ script = recipes/izakaya_barbecue_triggered_name slot_1 = {:s} slot_2 = {:s} slot_3 = {:s} slot_4 = {:s} slot_5 = {:s} recipe_result = {:s} }}\n";
    static constexpr auto inline_init_map = "set_variable = {{ which = izakaya_array_foods_count_{:s} value = 0 }}\n";
	// izakaya_cook_result_ = {
	// 	icon = "gfx/interface/izakaya/recipe/text/beef_yuanyang_hot_pot.dds"
	// 	custom_tooltip = izakaya_cook_result__tooltip show_only_custom_tooltip = no
	// }
	static constexpr auto static_modifier_result = "izakaya_cook_result_{:s}: {{ icon = \"gfx/interface/izakaya/recipe/text/{:s}.dds\" custom_tooltip = izakaya_cook_result_{:s}_tooltip show_only_custom_tooltip = no }}\n";
	static constexpr auto static_modifier_result_loc = " izakaya_cook_result_{:s}: \"$izakaya_food_{:s}$\"\n izakaya_cook_result_{:s}_tooltip: \"来源料理：['izakaya_food_{:s}']\"\n izakaya_cook_result_{:s}_desc: \"$izakaya_food_{:s}_desc$\"\n";
    for (auto& item : ingredients)
    {
        auto item_ingr = std::get<ingredient>(item);
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_name, item_ingr.key(), item_ingr.key(), item_ingr.name());
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_desc, item_ingr.key(), item_ingr.desc());
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_prefix, item_ingr.key());
        for (size_t i = 0; i < item_ingr.results.size(); ++i)
        {
            auto result = item_ingr.results[i];
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "['izakaya_food_{:s}']", get_name(result, results_keys));
            if (i + 1 >= item_ingr.results.size())
                break;
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "、");
        }
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_endfix, item_ingr.key());
    }

    for (auto& item : foods)
    {
        auto item_food = std::get<food>(item);
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_name, item_food.key(), item_food.key(), item_food.name());
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_desc_prefix, item_food.key());
        for (size_t i = 0; i < item_food.results.size(); ++i)
        {
            auto res_idx = item_food.results[i];
            if (res_idx < 0)
                break;
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "$building_izakaya_ingredient_{:s}$", ingredients_keys[res_idx].c_str());
            if (i + 1 < item_food.results.size() && item_food.results[i + 1] >= 0)
                std::format_to(std::ostreambuf_iterator<char>(loc_file), "、");
        }
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_desc_endfix, item_food.desc());
		// now also generate the static modifier localization for the food result.
		// the tooltip is empty for now, but it can be filled in later if needed.
		std::format_to(std::ostreambuf_iterator<char>(loc_file), static_modifier_result_loc, item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key());
    }
    loc_file.close();

    std::ofstream inline_file("./izakaya_barbecue_triggered_names.txt");
    for (auto recipe : recipes)
    {
        std::format_to(std::ostreambuf_iterator<char>(inline_file), inline_district_names, recipe.slot_key(0), recipe.slot_key(1), recipe.slot_key(2), recipe.slot_key(3), recipe.slot_key(4), recipe.key());
    }
    inline_file.close();

    inline_file.open("./izakaya_init_variable_array.txt");
    for (auto recipe : recipes)
    {
        std::format_to(std::ostreambuf_iterator<char>(inline_file), inline_init_map, recipe.key());
    }
    inline_file.close();
    return 0;
}
