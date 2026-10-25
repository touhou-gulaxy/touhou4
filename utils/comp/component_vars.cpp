// 从组件 CSV 生成属性、资源成本与缩放系数定义。
//
// 用法：
//   component_vars [--dir <mod>/utils]
//
// 输出：
//   components_attributes.log
//   components_resources.log
//   components_scales.log

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("无法读取: " + path.string());
    std::ostringstream buffer;
    buffer << in.rdbuf();

    std::string content = buffer.str();
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }
    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
    return content;
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("无法写入: " + path.string());
    out << content;
    if (!out) throw std::runtime_error("写入失败: " + path.string());
}

std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream in(content);
    while (std::getline(in, line)) lines.push_back(line);
    if (!content.empty() && content.back() == '\n') lines.emplace_back();
    return lines;
}

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    for (char c : line) {
        if (c == ',') {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    fields.push_back(field);
    return fields;
}

std::string trim(std::string_view value) {
    size_t begin = 0;
    size_t end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return std::string(value.substr(begin, end - begin));
}

float parseFloat(const std::string& value, size_t lineNumber, size_t column) {
    size_t parsed = 0;
    try {
        const float result = std::stof(value, &parsed);
        if (parsed != value.size()) throw std::invalid_argument("trailing characters");
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error(
            "CSV 第 " + std::to_string(lineNumber) + " 行第 " +
            std::to_string(column) + " 列不是有效浮点数: " + value);
    }
}

int parseInt(const std::string& value, size_t lineNumber, size_t column) {
    size_t parsed = 0;
    try {
        const int result = std::stoi(value, &parsed);
        if (parsed != value.size()) throw std::invalid_argument("trailing characters");
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error(
            "CSV 第 " + std::to_string(lineNumber) + " 行第 " +
            std::to_string(column) + " 列不是有效整数: " + value);
    }
}

// std::to_chars 生成最短浮点表示；补齐 Kotlin Float.toString 的 ".0" 形式。
std::string formatFloat(float value) {
    char buffer[64];
    const auto result = std::to_chars(
        std::begin(buffer), std::end(buffer), value, std::chars_format::general);
    if (result.ec != std::errc{}) throw std::runtime_error("无法格式化浮点数");

    std::string text(buffer, result.ptr);
    const size_t exponent = text.find_first_of("eE");
    if (exponent != std::string::npos) {
        std::string mantissa = text.substr(0, exponent);
        if (mantissa.find('.') == std::string::npos) mantissa += ".0";

        std::string exp = text.substr(exponent + 1);
        char sign = '+';
        if (!exp.empty() && (exp.front() == '+' || exp.front() == '-')) {
            sign = exp.front();
            exp.erase(exp.begin());
        }
        while (exp.size() > 1 && exp.front() == '0') exp.erase(exp.begin());
        if (exp.empty()) exp = "0";

        text = mantissa + "E" + (sign == '-' ? "-" : "") + exp;
    } else if (text.find('.') == std::string::npos) {
        text += ".0";
    }
    return text;
}

void requireColumns(const std::vector<std::string>& fields, size_t expected, size_t lineNumber) {
    if (fields.size() != expected) {
        throw std::runtime_error(
            "CSV 第 " + std::to_string(lineNumber) + " 行应有 " +
            std::to_string(expected) + " 列，实际为 " + std::to_string(fields.size()) + " 列");
    }
}

struct ComponentResource {
    std::string componentClass;
    std::string size;
    std::string type;
    float costAlloys;
    float costSpirit;
    float costFuka;
    float upkeepEnergy;
    float upkeepSpirit;
    float upkeepFuka;

    std::string toDefinition() const {
        const std::string prefix =
            "spth_" + componentClass + "_" + size + "_" + type;
        return
            "@" + prefix + "_cost_alloys = " + formatFloat(costAlloys) + "\n" +
            "@" + prefix + "_cost_sr_lingli = " + formatFloat(costSpirit) + "\n" +
            "@" + prefix + "_cost_sr_fuka = " + formatFloat(costFuka) + "\n" +
            "@" + prefix + "_upkeep_energy = " + formatFloat(upkeepEnergy) + "\n" +
            "@" + prefix + "_upkeep_sr_lingli = " + formatFloat(upkeepSpirit) + "\n" +
            "@" + prefix + "_upkeep_sr_fuka = " + formatFloat(upkeepFuka);
    }
};

struct ComponentAttribute {
    std::string name;
    std::string tag;
    std::string type;
    float damageMin;
    float damageMax;
    float windupMin;
    float windupMax;
    float fireTime;
    float minRange;
    float range;
    float accuracy;
    float tracking;
    float shieldPenetration;
    float armorPenetration;
    float shieldDamage;
    float armorDamage;
    float hullDamage;
    int power;

    std::string toDefinition() const {
        const std::string prefix = "spth_" + name + "_" + tag + "_" + type;
        return
            "@" + prefix + "_damage_min = " + formatFloat(damageMin) + "\n" +
            "@" + prefix + "_damage_max = " + formatFloat(damageMax) + "\n" +
            "@" + prefix + "_windup_min = " + formatFloat(windupMin) + "\n" +
            "@" + prefix + "_windup_max = " + formatFloat(windupMax) + "\n" +
            "@" + prefix + "_fire_time = " + formatFloat(fireTime) + "\n" +
            "@" + prefix + "_min_range = " + formatFloat(minRange) + "\n" +
            "@" + prefix + "_range = " + formatFloat(range) + "\n" +
            "@" + prefix + "_accuracy = " + formatFloat(accuracy) + "\n" +
            "@" + prefix + "_tracking = " + formatFloat(tracking) + "\n" +
            "@" + prefix + "_shield_penetration = " + formatFloat(shieldPenetration) + "\n" +
            "@" + prefix + "_armor_penetration = " + formatFloat(armorPenetration) + "\n" +
            "@" + prefix + "_shield_damage = " + formatFloat(shieldDamage) + "\n" +
            "@" + prefix + "_armor_damage = " + formatFloat(armorDamage) + "\n" +
            "@" + prefix + "_hull_damage = " + formatFloat(hullDamage) + "\n" +
            "@" + prefix + "_power = " + std::to_string(power);
    }
};

std::vector<ComponentResource> readComponentResources(const fs::path& path) {
    std::vector<ComponentResource> resources;
    const auto lines = splitLines(readFile(path));
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].empty() || lines[i].front() == '#') continue;
        const auto fields = splitCsv(lines[i]);
        requireColumns(fields, 9, i + 1);
        resources.push_back({
            fields[0], fields[1], fields[2],
            parseFloat(fields[3], i + 1, 4),
            parseFloat(fields[4], i + 1, 5),
            parseFloat(fields[5], i + 1, 6),
            parseFloat(fields[6], i + 1, 7),
            parseFloat(fields[7], i + 1, 8),
            parseFloat(fields[8], i + 1, 9),
        });
    }
    return resources;
}

std::vector<ComponentAttribute> readComponentAttributes(
    const fs::path& path, std::set<std::string>& types) {
    std::vector<ComponentAttribute> attributes;
    const auto lines = splitLines(readFile(path));
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].empty() || lines[i].front() == '#') continue;
        const auto fields = splitCsv(lines[i]);
        requireColumns(fields, 18, i + 1);
        ComponentAttribute attribute{
            fields[0], fields[1], fields[2],
            parseFloat(fields[3], i + 1, 4),
            parseFloat(fields[4], i + 1, 5),
            parseFloat(fields[5], i + 1, 6),
            parseFloat(fields[6], i + 1, 7),
            parseFloat(fields[7], i + 1, 8),
            parseFloat(fields[8], i + 1, 9),
            parseFloat(fields[9], i + 1, 10),
            parseFloat(fields[10], i + 1, 11),
            parseFloat(fields[11], i + 1, 12),
            parseFloat(fields[12], i + 1, 13),
            parseFloat(fields[13], i + 1, 14),
            parseFloat(fields[14], i + 1, 15),
            parseFloat(fields[15], i + 1, 16),
            parseFloat(fields[16], i + 1, 17),
            parseInt(fields[17], i + 1, 18),
        };
        types.insert(attribute.type);
        attributes.push_back(std::move(attribute));
    }
    return attributes;
}

std::map<std::string, std::string> parseExistingDefinitions(const fs::path& path) {
    std::map<std::string, std::string> definitions;
    if (!fs::exists(path)) return definitions;

    const auto lines = splitLines(readFile(path));
    for (const auto& raw : lines) {
        std::string line = raw.substr(0, raw.find('#'));
        line = trim(line);
        if (line.empty() || line.front() != '@') continue;
        const size_t equals = line.find('=');
        if (equals == std::string::npos || equals <= 1) continue;
        const std::string name = trim(std::string_view(line).substr(1, equals - 1));
        const std::string value = trim(std::string_view(line).substr(equals + 1));
        definitions[name] = value;
    }
    return definitions;
}

fs::path parseBaseDirectory(int argc, char** argv) {
    fs::path directory = fs::current_path();
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--dir") {
            if (i + 1 >= argc) throw std::runtime_error("--dir 缺少路径");
            directory = argv[++i];
        } else if (argument == "-h" || argument == "--help") {
            std::cout << "用法: component_vars [--dir <mod>/utils]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("未知参数: " + argument);
        }
    }
    return fs::absolute(directory).lexically_normal();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const fs::path baseDirectory = parseBaseDirectory(argc, argv);
        const fs::path attributeCsv = baseDirectory / "components_attributes.csv";
        const fs::path resourceCsv = baseDirectory / "components_resources.csv";
        const fs::path targetVariables =
            baseDirectory / ".." / "common" / "scripted_variables" /
            "spth_component_variables.txt";
        const fs::path outputAttributes = baseDirectory / "components_attributes.log";
        const fs::path outputResources = baseDirectory / "components_resources.log";
        const fs::path outputScales = baseDirectory / "components_scales.log";

        std::set<std::string> types;
        const auto attributes = readComponentAttributes(attributeCsv, types);
        const auto resources = readComponentResources(resourceCsv);

        std::string attributeOutput;
        for (const auto& attribute : attributes) {
            attributeOutput += attribute.toDefinition();
            attributeOutput += '\n';
        }
        writeFile(outputAttributes, attributeOutput);

        std::string resourceOutput;
        for (const auto& resource : resources) {
            resourceOutput += resource.toDefinition();
            resourceOutput += '\n';
        }
        writeFile(outputResources, resourceOutput);

        const auto existing = parseExistingDefinitions(targetVariables);
        std::string scaleOutput;
        for (const auto& type : types) {
            for (const std::string_view kind : {"damage", "windup", "fire_time"}) {
                const std::string key =
                    "touhou_generic_scale_" + std::string(kind) + "_" + type + "_factor";
                const auto found = existing.find(key);
                const std::string value = found == existing.end() ? "1.0" : found->second;
                scaleOutput += "@" + key + " = " + value + "\n";
            }
        }
        writeFile(outputScales, scaleOutput);

        std::cout << "generated: " << fs::absolute(outputAttributes).lexically_normal()
                  << " (" << types.size() << " types)\n";
        std::cout << "generated: " << fs::absolute(outputResources).lexically_normal() << '\n';
        std::cout << "generated: " << fs::absolute(outputScales).lexically_normal()
                  << " (values taken from "
                  << fs::absolute(targetVariables).lexically_normal() << ")\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
