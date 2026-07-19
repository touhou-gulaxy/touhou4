#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <array>
#include <cstdint>
#include <variant>
#include <format>

#define SPECIAL_TIER_REISEN 72

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
	int_fast8_t tier = 1;

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

    const char* name() const
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
	
	int ingredient_count() const
	{
		// the results vector contains the index = -1 which indicates null ingredient, so we need to count the number of valid ingredients.
		return std::count_if(results.begin(), results.end(), [](int_fast8_t idx) { return idx >= 0; });
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
	auto start_time = std::chrono::high_resolution_clock::now();
    build_maps("./izakaya_ingredients_names.txt", "izakaya_ingredients_maps.txt");
    build_maps("./izakaya_results_names.txt", "izakaya_results_maps.txt", false);
    ingredients_descs.resize(ingredients_keys.size());
    foods_descs.resize(results_keys.size());

    // izakaya_ingredients_descs.txt
    auto recipes = build_recipes("./recipes.txt");
    auto ingredients = build_izakaya_items("./izakaya_ingredients_descs.txt");
    auto foods = build_izakaya_items<false>("./izakaya_results_descs.txt");
	double elapsed_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count();
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
    std::sort(recipes.begin(), recipes.end(),
        [](const recipe& a, const recipe& b) {
            auto valid = [](const recipe& r) {
                return std::count_if(r.slots.begin(), r.slots.end(),
                    [](int_fast8_t s) { return s != -1; });
            };
            int va = valid(a), vb = valid(b);
            if (va != vb) return va > vb;
            return a.result < b.result;
        });

	start_time = std::chrono::high_resolution_clock::now();
    printf("scaned %lu recipes, indexed %lu ingredients and %lu foods in %.4f ms.\n", recipes.size(), ingredients.size(), foods.size(), elapsed_time);

	size_t elapsed_count = 0;
	// scan the izakaya_ingredients_tiers.txt file, to get the ingredient tiers, and update the ingredient objects accordingly.
	// file format: [key]:[tier], like eggs:1.
    std::ifstream tier_file("./izakaya_ingredients_tiers.txt");
    if (tier_file.is_open())
    {
        std::string line;
        while (std::getline(tier_file, line))
        {
			if(line.empty())
				continue;
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
				++elapsed_count;
                std::string key = line.substr(0, colon_pos);
				// if tier part matches string "reisen", set the tier to SPECIAL_TIER_REISEN, otherwise parse the tier as an integer.
				auto tier_str = line.substr(colon_pos + 1);
				int_fast8_t tier;
				if (tier_str == "reisen")
					tier = SPECIAL_TIER_REISEN;
				else
                	tier = std::stoi(line.substr(colon_pos + 1));
                auto iter = std::find_if(ingredients.begin(), ingredients.end(), [&key](const std::variant<ingredient, food>& item) {
                    return std::get<ingredient>(const_cast<std::variant<ingredient, food>&>(item)).key() == key;
                });
                if (iter != ingredients.end())
                {
                    std::get<ingredient>(*iter).tier = tier;
                }
            }
        }
        tier_file.close();
    }

#if 0
	// debug print all the tiers.
	for (auto& item : ingredients)
	{
		auto& item_ingr = std::get<ingredient>(item);
		printf("ingredient %s has %d tier\n", item_ingr.key(), item_ingr.tier);
	}
#endif

	using tier_color_pair = std::pair<int_fast8_t, char>;
	using tier_food_title_pair = std::pair<int_fast8_t, const char*>;
	static constexpr auto tier_colors = {
		tier_color_pair(1, 'Y'),
		tier_color_pair(2, 'r'),
		tier_color_pair(3, 'l'),
		tier_color_pair(4, 'U'),
		tier_color_pair(SPECIAL_TIER_REISEN, 'B')
	};
	static constexpr auto tier_food_titles = {
		tier_food_title_pair(1, ""),
	};
	static auto calc_food_tier = [ingredients](food f) -> int_fast8_t
	{
		// the tier of the food should be the highest tier of the ingredients used to make it.
		int_fast8_t max_tier = 1;
		for (const auto& ingr : f.results)
		{
			auto iter = std::find_if(ingredients.begin(), ingredients.end(), [ingr](const std::variant<ingredient, food>& item) {
				return std::get<ingredient>(const_cast<std::variant<ingredient, food>&>(item)).id == ingr;
			});
			if (iter != ingredients.end())
			{
				max_tier = std::max(max_tier, std::get<ingredient>(*iter).tier);
			}
		}
		return max_tier;
	};

    std::ofstream loc_file("./izakaya_locs.yml");
    std::format_to(std::ostreambuf_iterator<char>(loc_file), "l_simp_chinese:\n # This file is auto generated by process.cpp, do not edit manually.\n # tier 72 is special for reisen.\n");
	for (auto color_pair : tier_colors)
	{
		std::format_to(std::ostreambuf_iterator<char>(loc_file), " IZAKAYA_TIER_{:d}_COLOR_PREFIX: \"§{:c}\"\n", color_pair.first, color_pair.second);
		++elapsed_count;
	}
    std::format_to(std::ostreambuf_iterator<char>(loc_file), " IZAKAYA_TIER_COLOR_ENDFIX: \"§!\"\n\n");
    //  building_izakaya_ingredient_tofu: "£izakaya_ingredients_item_tofu£豆腐"
    //  building_izakaya_ingredient_tofu_desc: ""
    //  building_izakaya_ingredient_tofu_tt: "\n食材数量：§Y[owner.izakaya_ingredient_tofu_val]/[get_izakaya_ingredient_max_val]§!"
    static constexpr auto building_loc_name = " building_izakaya_ingredient_{:s}: \"£izakaya_ingredients_item_{:s}£$IZAKAYA_TIER_{:d}_COLOR_PREFIX${:s}$IZAKAYA_TIER_COLOR_ENDFIX$\"\n";
    static constexpr auto building_loc_desc = " building_izakaya_ingredient_{:s}_desc: \"{:s}\"\n";
    static constexpr auto building_loc_tooltip_reisen = " building_izakaya_ingredient_{:s}_tt: \"不可用于合成，适合拍照（？）";
    static constexpr auto building_loc_tooltip_prefix = " building_izakaya_ingredient_{:s}_tt: \"关联料理：";
    static constexpr auto building_loc_tooltip_endfix = "\\n食材数量：§Y[owner.izakaya_ingredient_{:s}_val]/[get_izakaya_ingredient_max_val]§!\"\n";
    //  izakaya_food_tsukimi_mochi: "£izakaya_result_foods_item_tsukimi_mochi£月见饼"
    //  izakaya_food_tsukimi_mochi_desc: ""
    static constexpr auto concept_loc_name = " izakaya_food_{:s}: \"£izakaya_result_foods_item_{:s}£{:s}\"\n";
    static constexpr auto concept_loc_desc_prefix = " izakaya_food_{:s}_desc: \"需求：§Y烧烤架§!\\n原材料：";
    static constexpr auto concept_loc_desc_endfix = "\\n{:s}\"\n";
	// district_izakaya_barbecue_result_fantasy_craze: "制作料理：['izakaya_food_fantasy_craze']"
	static constexpr auto district_loc_result = " district_izakaya_barbecue_result_{:s}: \"制作料理：['izakaya_food_{:s}']\"\n";
	static constexpr auto event_loc_recipe_book_item = " izakaya_recipe_book_{:s}_response_text: \"$izakaya_recipe.1000.desc.prefix$$izakaya_food_{:s}$\\n$izakaya_food_{:s}_desc$\\n预计有§Y[get_izakaya_recipe_{:s}_success_chance].00%§!概率制作成功，制作时间为[from.get_izakaya_recipe_{:s}_complete_time]£time£。\"\n";
    // inline_script = {
	//     script = recipes/izakaya_barbecue_triggered_name recipe_result = fantasy_craze
	//     slot_1 = potatoes slot_2 = honey slot_3 = null slot_4 = null slot_5 = null
    //  }
    static constexpr auto inline_district_names = "inline_script = {{ script = recipes/izakaya_barbecue_triggered_name slot_1 = {:s} slot_2 = {:s} slot_3 = {:s} slot_4 = {:s} slot_5 = {:s} recipe_result = {:s} }}\n";
    static constexpr auto inline_init_map = "set_variable = {{ which = izakaya_array_foods_count_{:s} value = 0 }}\n";
	static constexpr auto inline_finish_cook = "inline_script = {{ script = recipes/izakaya_finish_cook_food type = {:s} }}\n";
	// izakaya_cook_result_ = {
	// 	icon = "gfx/interface/izakaya/recipe/text/beef_yuanyang_hot_pot.dds"
	// 	custom_tooltip = izakaya_cook_result__tooltip show_only_custom_tooltip = no
	// }
	static constexpr auto static_modifier_result = "izakaya_cook_result_{:s} = {{\n    icon = \"gfx/interface/izakaya/recipe/text/{:s}.dds\" custom_tooltip = izakaya_cook_result_{:s}_tooltip show_only_custom_tooltip = no\n\n}}\n";
	static constexpr auto static_modifier_result_loc = " izakaya_cook_result_{:s}: \"$izakaya_food_{:s}$\"\n izakaya_cook_result_{:s}_tooltip: \"来源料理：['izakaya_food_{:s}']\"\n izakaya_cook_result_{:s}_desc: \"$izakaya_food_{:s}_desc$\"\n";
	static constexpr auto scripted_variable_food_tiers = "@izakaya_food_tier_{:s} = {:d}\n";
	// @izakaya_food_ingredient_count_{food_key} = %d(numbers of ingredients used to make the food)
	static constexpr auto scripted_variable_food_ingredient_count = "@izakaya_food_ingredient_count_{:s} = {:d}\n";
	// @izakaya_recipe_detail_item_eggs = "izakaya_has_ingredient = { type = eggs }"
	static constexpr auto scripted_variable_recipe_detail = "@izakaya_recipe_detail_item_{:s} = \"izakaya_has_ingredient = {{ type = {:s} }}\"\n";
	// @izakaya_recipe_detail_has_item_eggs = "izakaya_has_built_ingredient = { type = eggs }"
	static constexpr auto scripted_variable_recipe_detail_has = "@izakaya_recipe_detail_has_item_{:s} = \"izakaya_has_built_ingredient = {{ type = {:s} }}\"\n";
	static constexpr auto effect_recipe_detail_apply = "set_country_flag = izakaya_ingredient_display_recipe_show_{:s} ";
	// if/else_if = { inline_script = { script = recipes/izakaya_init_cook_food_item slot_1 = {:s} slot_2 = {:s} slot_3 = {:s} slot_4 = {:s} slot_5 = {:s} food = {:s} } }
	static constexpr auto inline_init_cook_food_item = "{:s} = {{ inline_script = {{ script = recipes/izakaya_init_cook_food_item slot_1 = {:<23} slot_2 = {:<23} slot_3 = {:<23} slot_4 = {:<23} slot_5 = {:<23} food = {:s} }} }}\n";
	// country_event = {
	// 	id = izakaya_recipe.{:d} hide_window = yes is_triggered_only = yes
	// 	immediate = {
	// 		if = { limit = { check_variable = { which = block_izakaya_ingredient_allow_mutex value = 0 } } remove_country_flag = block_izakaya_ingredient_allow }
	// 		inline_script = { script = recipes/izakaya_finish_cook_food type = {:s} }
	// 	}
	// }
	static constexpr auto event_finish_cook_food = "country_event = {{ id = izakaya_recipe.{:d} hide_window = yes is_triggered_only = yes immediate = {{ change_variable = {{ which = block_izakaya_ingredient_allow_mutex value = -1 }} if = {{ limit = {{ check_variable = {{ which = block_izakaya_ingredient_allow_mutex value = 0 }} }} remove_country_flag = block_izakaya_ingredient_allow }} inline_script = {{ script = recipes/izakaya_finish_cook_food food = {:s} }} }} }}\n";
	static constexpr auto event_finish_cook_food_failed = "country_event = {{ id = izakaya_recipe.{:d} hide_window = yes is_triggered_only = yes immediate = {{ change_variable = {{ which = block_izakaya_ingredient_allow_mutex value = -1 }} if = {{ limit = {{ check_variable = {{ which = block_izakaya_ingredient_allow_mutex value = 0 }} }} remove_country_flag = block_izakaya_ingredient_allow }} inline_script = {{ script = recipes/izakaya_finish_cook_food_failed food = {:s} }} }} }}\n";
	static constexpr auto event_recipe_book_option = "	option = {{\n		name = izakaya_food_{:s}\n		custom_tooltip = izakaya_food_{:s}_desc\n		trigger = {{ check_variable = {{ which = izakaya_recipe_book_1000_mode value = @izakaya_recipe_book_display_mode_{:s} }}{:s} }}\n		response_text = izakaya_recipe_book_{:s}_response_text\n		is_dialog_only = yes\n		hidden_effect = {{ inline_script = {{ script = recipes/izakaya_set_recipe_book_select food = {:s} }} }}\n	}}\n";
	static constexpr auto sloc_recipe_success_chance = "define_text = {{ name = get_izakaya_recipe_{:s}_success_chance value = value:country_izakaya_food_success_chance|food|{:s}| }}\n";
	static constexpr auto sloc_recipe_complete_time = "define_text = {{ name = get_izakaya_recipe_{:s}_complete_time value = value:country_izakaya_food_making_time|food|{:s}| }}\n";
    static constexpr auto loc_recipe_success_chance = " izakaya_recipe_{:s}_success_chance: \"\\n预期制作的料理为['izakaya_food_{:s}']，有§Y[get_izakaya_recipe_{:s}_success_chance].00%§!概率制作成功。\"\n";
    static constexpr auto loc_recipe_complete_time = " izakaya_recipe_{:s}_complete_time: \"预期制作花费时间：$TOUHOU_TIME|Y$§Y.00§!£time£\"\n";

	// find ingredient with name "铃仙", and print special tooltip for it.
	auto reisen_iter = std::find_if(ingredients.begin(), ingredients.end(), [](std::variant<ingredient, food>& item) {
		auto& ingr = const_cast<ingredient&>(std::get<ingredient>(item));
		return std::strcmp(ingr.name(), "铃仙") == 0;
	});
	bool reisen_found = reisen_iter != ingredients.end();
    for (auto& item : ingredients)
    {
		++elapsed_count;
        auto item_ingr = std::get<ingredient>(item);
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_name, item_ingr.key(), item_ingr.key(), item_ingr.tier, item_ingr.name());
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_desc, item_ingr.key(), item_ingr.desc());
		if (reisen_found && item_ingr.id == std::get<ingredient>(*reisen_iter).id)
		{
			std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_reisen, item_ingr.key());
        	std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_endfix, item_ingr.key());
			continue;
		}
		else
        	std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_prefix, item_ingr.key());
		auto item_food_ref_count = 0;
        for (size_t i = 0; i < item_ingr.results.size(); ++i)
        {
			++elapsed_count;
            auto result = item_ingr.results[i];
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "['izakaya_food_{:s}']", get_name(result, results_keys));
            if (i + 1 >= item_ingr.results.size())
                break;
			++item_food_ref_count;
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "、");
        }
		if (item_food_ref_count == 0)
			std::format_to(std::ostreambuf_iterator<char>(loc_file), "无");
        std::format_to(std::ostreambuf_iterator<char>(loc_file), building_loc_tooltip_endfix, item_ingr.key());
    }

    for (auto& item : foods)
    {
		++elapsed_count;
        auto item_food = std::get<food>(item);
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_name, item_food.key(), item_food.key(), item_food.name());
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_desc_prefix, item_food.key());
        for (size_t i = 0; i < item_food.results.size(); ++i)
        {
			++elapsed_count;
            auto res_idx = item_food.results[i];
            if (res_idx < 0)
                break;
            std::format_to(std::ostreambuf_iterator<char>(loc_file), "$building_izakaya_ingredient_{:s}$", ingredients_keys[res_idx].c_str());
            if (i + 1 < item_food.results.size() && item_food.results[i + 1] >= 0)
                std::format_to(std::ostreambuf_iterator<char>(loc_file), "、");
        }
        std::format_to(std::ostreambuf_iterator<char>(loc_file), concept_loc_desc_endfix, item_food.desc());
		std::format_to(std::ostreambuf_iterator<char>(loc_file), district_loc_result, item_food.key(), item_food.key());
		std::format_to(std::ostreambuf_iterator<char>(loc_file), event_loc_recipe_book_item, item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key());
		// now also generate the static modifier localization for the food result.
		// the tooltip is empty for now, but it can be filled in later if needed.
		std::format_to(std::ostreambuf_iterator<char>(loc_file), static_modifier_result_loc, item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key(), item_food.key());
    }

	for (auto& item : recipes)
	{
		++elapsed_count;
        std::format_to(std::ostreambuf_iterator<char>(loc_file), loc_recipe_success_chance, item.key(), item.key(), item.key());
		std::format_to(std::ostreambuf_iterator<char>(loc_file), loc_recipe_complete_time, item.key());
	}
    loc_file.close();

    std::ofstream inline_file("./izakaya_barbecue_triggered_names.txt");
    for (auto recipe : recipes)
    {
		++elapsed_count;
        std::format_to(std::ostreambuf_iterator<char>(inline_file), inline_district_names, recipe.slot_key(0), recipe.slot_key(1), recipe.slot_key(2), recipe.slot_key(3), recipe.slot_key(4), recipe.key());
    }
    inline_file.close();

    inline_file.open("./izakaya_init_variable_array.txt");
    for (auto recipe : recipes)
    {
		++elapsed_count;
        std::format_to(std::ostreambuf_iterator<char>(inline_file), inline_init_map, recipe.key());
    }
    inline_file.close();

	inline_file.open("./event/izakaya_finish_cook_switch.txt");
	for (auto recipe : recipes)
	{
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(inline_file), inline_finish_cook, recipe.key());
	}
	inline_file.close();
	inline_file.open("./event/izakaya_set_recipe_book_select.txt");
	for (auto item : foods)
	{
		++elapsed_count;
		auto f = std::get<food>(item);
		std::format_to(std::ostreambuf_iterator<char>(inline_file), "if = {{ limit = {{ has_country_flag = izakaya_recipe_book_selected_{:s} }} remove_country_flag = izakaya_recipe_book_selected_{:s} }}\n", f.key(), f.key());
	}
	std::format_to(std::ostreambuf_iterator<char>(inline_file), "set_country_flag = izakaya_recipe_book_selected_$food$\n");
	inline_file.close();

	std::ofstream static_mod_file("./izakaya_static_modifiers.txt");
	for (auto recipe : recipes)
	{
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(static_mod_file), static_modifier_result, recipe.key(), recipe.key(), recipe.key());
	}
	static_mod_file.close();

	std::ofstream scripted_var_file("./izakaya_scripted_variables.txt");
	std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "# This file is auto generated by process.cpp, do not edit manually.\n@izakaya_food_tier_constant_t1 = 1\n@izakaya_food_tier_constant_t2 = 2\n@izakaya_food_tier_constant_t3 = 3\n@izakaya_food_tier_constant_t4 = 4\n@izakaya_food_tier_constant_t72 = 72\n\n");
	for (auto item : foods)
	{
		auto f = std::get<food>(item);
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), scripted_variable_food_tiers, f.key(), calc_food_tier(f));
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), scripted_variable_food_ingredient_count, f.key(), f.ingredient_count());
	}
	// put fallback variables:
	// @izakaya_recipe_detail_item_null = "touhou_always = { always = yes }"
	// @izakaya_recipe_detail_has_item_null = "touhou_always = { always = yes }"
	std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "\n@izakaya_recipe_detail_item_null = \"touhou_always = {{ always = yes }}\"\n");
	std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "@izakaya_recipe_detail_has_item_null = \"touhou_always = {{ always = yes }}\"\n");
	// puts the ingredients into the scripted variable file, so that we can use them in the event scripts.
	for (auto item : ingredients)
	{
		auto ingr = std::get<ingredient>(item);
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), scripted_variable_recipe_detail, ingr.key(), ingr.key());
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), scripted_variable_recipe_detail_has, ingr.key(), ingr.key());
	}

	std::ofstream effect_file("./event/izakaya_be_start_cook_food.txt");
	auto iter = recipes.begin();
	// the first item begins with "if", and others are "else_if".
	if (iter != recipes.end())
	{
		std::format_to(std::ostreambuf_iterator<char>(effect_file), inline_init_cook_food_item, "if     ", iter->slot_key(0), iter->slot_key(1), iter->slot_key(2), iter->slot_key(3), iter->slot_key(4), iter->key());
		++iter;
	}
	while (iter != recipes.end())
	{
		std::format_to(std::ostreambuf_iterator<char>(effect_file), inline_init_cook_food_item, "else_if", iter->slot_key(0), iter->slot_key(1), iter->slot_key(2), iter->slot_key(3), iter->slot_key(4), iter->key());
		++iter;
		++elapsed_count;
	}
	effect_file.close();
	// effect_recipe_detail_apply
	effect_file.open("./event/izakaya_recipes_apply.txt");
	for (auto r : recipes)
	{
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(effect_file), "izakaya_recipe_book_selected_{:s} = {{ ", r.key());
		for (int_fast8_t i = 0; i < 5; ++i)
		{
			if (r.slots[i] < 0)
				break;
			std::format_to(std::ostreambuf_iterator<char>(effect_file), effect_recipe_detail_apply, r.slot_key(i));
		}
		std::format_to(std::ostreambuf_iterator<char>(effect_file), "}}\n");
	}
	effect_file.close();
	effect_file.open("./event/izakaya_recipes_clear.txt");
	for (auto item : ingredients)
	{
		++elapsed_count;
		auto ingr = std::get<ingredient>(item);
		std::format_to(std::ostreambuf_iterator<char>(effect_file), "if = {{ limit = {{ has_country_flag = izakaya_ingredient_display_recipe_show_{:s} }} remove_country_flag = izakaya_ingredient_display_recipe_show_{:s} }}\n", ingr.key(), ingr.key());

	}
	for (auto r : recipes)
	{
		++elapsed_count;
		std::format_to(std::ostreambuf_iterator<char>(effect_file), "if = {{ limit = {{ has_country_flag = izakaya_recipe_book_selected_{:s} }} remove_country_flag = izakaya_recipe_book_selected_{:s} }}\n", r.key(), r.key());
	}
	effect_file.close();

	int event_id = 1;
	std::ofstream event_file("./event/izakaya_recipe_events.txt");
	std::ofstream sloc_file("./event/izakaya_recipe_locs.txt");
	std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "namespace = izakaya_recipe\n# This file is auto generated by process.cpp, do not edit manually.\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "# izakaya_recipe.{:d} - izakaya_recipe.{:d}: finish recipe events.\n\n", event_id, 1 + recipes.size());
	std::format_to(std::ostreambuf_iterator<char>(event_file), "# izakaya_recipe.{:d} - izakaya_recipe.{:d}: finish recipe but failed events.\n\n", event_id + recipes.size() + 1, event_id + 1 + recipes.size() * 2);
	std::format_to(std::ostreambuf_iterator<char>(sloc_file), "# This file is auto generated by process.cpp, do not edit manually.\n\n");
	for (auto item : recipes)
	{
		elapsed_count += 4;
		std::format_to(std::ostreambuf_iterator<char>(event_file), event_finish_cook_food, event_id, item.key());
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "@izakaya_food_{:s}_finish_event_call = \"izakaya_recipe.{:d}\"\n", item.key(), event_id++);
		std::format_to(std::ostreambuf_iterator<char>(sloc_file), sloc_recipe_success_chance, item.key(), item.key());
		std::format_to(std::ostreambuf_iterator<char>(sloc_file), sloc_recipe_complete_time, item.key(), item.key());
	}
	for (auto item : recipes)
	{
		elapsed_count += 2;
		std::format_to(std::ostreambuf_iterator<char>(event_file), event_finish_cook_food_failed, event_id, item.key());
		std::format_to(std::ostreambuf_iterator<char>(scripted_var_file), "@izakaya_food_{:s}_finish_failed_event_call = \"izakaya_recipe.{:d}\"\n", item.key(), event_id++);
	}
	std::format_to(std::ostreambuf_iterator<char>(event_file), "\n\ncountry_event = {{\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "	inline_script = {{ script = recipes/izakaya_recipe_book_start id = 1000 }}\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "	trigger = {{ NOT = {{ has_country_flag = izakaya_recipe_book_1000_open }} }}\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "	immediate = {{ set_country_flag = izakaya_recipe_book_1000_open if = {{ limit = {{ NOT = {{ is_variable_set = izakaya_recipe_book_1000_mode }} }} set_variable = {{ which = izakaya_recipe_book_1000_mode value = @izakaya_recipe_book_display_mode_all }} }} }}\n");
	std::format_to(std::ostreambuf_iterator<char>(event_file), "	after = {{ remove_country_flag = izakaya_recipe_book_1000_open }}\n");
	for (auto item : recipes)
	{
		elapsed_count += 5;
		std::string allow_cond = "", disallow_cond = "";
		for (int_fast8_t i = 0; i < 5; ++i)
		{
			elapsed_count += 2;
			if (item.slots[i] < 0)
				break;
			allow_cond += std::format(" check_variable = {{ which = izakaya_ingredient_{:s}_val value > 0 }}", item.slot_key(i));
			disallow_cond += std::format(" check_variable = {{ which = izakaya_ingredient_{:s}_val value <= 0 }}", item.slot_key(i));
		}
		auto tier = calc_food_tier(std::get<food>(foods[item.result]));
		std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "all", "", item.key(), item.key());
		std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "allow", allow_cond.c_str(), item.key(), item.key());
		std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "disallow", disallow_cond.c_str(), item.key(), item.key());
		switch (tier)
		{
			case 1:
			std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "t1", "", item.key(), item.key());
			break;
			case 2:
			std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "t2", "", item.key(), item.key());
			break;
			case 3:
			std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "t3", "", item.key(), item.key());
			break;
			case 4:
			std::format_to(std::ostreambuf_iterator<char>(event_file), event_recipe_book_option, item.key(), item.key(), "t4", "", item.key(), item.key());
			break;
		}
	}
	std::format_to(std::ostreambuf_iterator<char>(event_file), "}}\n");
	event_file.close();
	scripted_var_file.close();
	sloc_file.close();

#if 0
	// generate debug event options.
	size_t idx = 0;
	for (auto recipe : recipes)
	{
		printf("option = { name = \"%zu item: %s\" }\n", idx++, recipe.key());
	}
#endif
	elapsed_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count();
	printf("finish %zu iterations in %.4f ms.\n", elapsed_count, elapsed_time);
    return 0;
}
