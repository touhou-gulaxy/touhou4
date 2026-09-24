// ags_psionic_variants.cpp
//
// 为 touhou4 的灵能舰船模型（gfx/models/ships/AGS_Ships 下的 _spth_psi_ags* 定义文件）
// 批量生成其它颜色的差分（mesh / entity 的注册），把 diffuse 贴图换成
// ags_psionic_<color>_diffuse.dds。blue 视为默认色（主定义文件里已经注册），默认跳过。
//
// 命名规则（默认）：
//   AGS_psi_Battleship_Basic_mesh
//     → lunar_capital_psi_event_red_diff_AGS_psi_Battleship_Basic_mesh
//   AGS_psi_Battleship_Standard_entity
//     → lunar_capital_psi_event_red_diff_AGS_psi_Battleship_Standard_entity
//   即把 "lunar_capital_psi_event_<color>_diff_" 直接拼在原始 key 前面，原始名字整体保留。
//   块内所有 AGS_psi_ 引用（含 attach = { ... = "AGS_psi_..." }）一并改名，
//   所有 ags_psionic_<任意色>_diffuse.dds 一并换成目标颜色。
//
// 为什么这样拼（便于 graphical_culture 换色）：
//   飞船实体的最终名字 = <graphical_culture 名> + "_" + <ship_size 里写的 entity 名>，
//   见反编译 stellaris_4.5_source.cpp:686731 CGfxCulture::GetEntity() /
//   GetAssetName()（逐个沿 fallback 链查 <culture>_<entity>，都找不到才退回裸名）。
//   所以只要定义一个名叫 lunar_capital_psi_event_<color>_diff 的 graphical_culture，
//   再把飞船（或国家）的 graphical_culture 设成它，就能换色；
//   原始 AGS_psi_* 名字保持不变，仍是 blue 默认图。
//   若想要短名字（把 AGS_psi_ 去掉），加 --strip-source-prefix。
//
// 构建：
//   g++ -std=c++17 -O2 -o ags_psionic_variants ags_psionic_variants.cpp
//
// 用法：
//   ./ags_psionic_variants                          # 就地生成到 mod 目录（默认）
//   ./ags_psionic_variants --dry-run                # 只统计/预览，不写文件
//   ./ags_psionic_variants --out /tmp/ags_test      # 输出到别处
//   ./ags_psionic_variants --colors red,green       # 只生成指定颜色
//   ./ags_psionic_variants --suffix _diff           # 输出文件后缀（默认 _diff）
//   ./ags_psionic_variants --name-prefix 'foo_{color}_'  # 自定义 key 前缀模板（{color} = 颜色）
//   ./ags_psionic_variants --strip-source-prefix    # 变体名里去掉 AGS_psi_ 前缀
//   ./ags_psionic_variants --list-keys keys.txt     # 额外导出 原名→变体名 对照表
//
// 默认目录：<mod 根>/gfx/models/ships/AGS_Ships
// （脚本按自身路径向上找不到 mod 根时，请用 --dir 显式指定）

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr const char* kKeyPrefix = "AGS_psi_";   // 需要变体化的 key 前缀（blue 默认名）
constexpr const char* kDefaultColor = "blue";    // 已作为默认注册，跳过
// 变体 key 前缀模板，{color} 会被替换成颜色名
constexpr const char* kDefaultNamePrefix = "lunar_capital_psi_event_{color}_diff_";
// 变体 key 的固定前缀（幂等检测用；--name-prefix 改动后此值只用于识别本工具旧输出）
constexpr const char* kGeneratedPrefix = "lunar_capital_psi_event_";

struct Options {
    fs::path dir;                 // 定义文件所在目录
    fs::path out;                 // 输出目录（空 = 与 dir 相同）
    std::set<std::string> colors; // 空 = 自动探测
    std::string suffix = "_diff"; // 输出文件后缀
    std::string namePrefix = kDefaultNamePrefix;  // key 前缀模板
    bool stripSourcePrefix = false;               // 变体名里去掉 AGS_psi_
    bool dryRun = false;
    fs::path listKeys;            // 可选：导出对照表
};

std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("无法读取: " + p.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFile(const fs::path& p, const std::string& content) {
    if (!p.parent_path().empty()) fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    if (!out) throw std::runtime_error("无法写入: " + p.string());
    out << content;
}

// 在文本里找出所有 "key = { ... }" 块（花括号配对），块范围包含 key 本身。
std::vector<std::pair<size_t, size_t>> findBlocks(const std::string& text, const std::string& key) {
    std::vector<std::pair<size_t, size_t>> blocks;
    size_t pos = 0;
    while (true) {
        size_t k = text.find(key, pos);
        if (k == std::string::npos) break;

        // 必须是行首（允许前置空白），避免匹配到别的单词中间
        size_t lineBegin = k;
        while (lineBegin > 0 && (text[lineBegin - 1] == ' ' || text[lineBegin - 1] == '\t')) --lineBegin;
        const bool atLineStart = (lineBegin == 0) || text[lineBegin - 1] == '\n';

        size_t eq = text.find('=', k + key.size());
        if (!atLineStart || eq == std::string::npos) {
            pos = k + key.size();
            continue;
        }
        size_t brace = text.find('{', eq);
        if (brace == std::string::npos) break;

        size_t depth = 0;
        size_t i = brace;
        for (; i < text.size(); ++i) {
            if (text[i] == '{') {
                ++depth;
            } else if (text[i] == '}') {
                if (--depth == 0) { ++i; break; }
            }
        }
        blocks.emplace_back(lineBegin, i);
        pos = i;
    }
    return blocks;
}

// 取块里第一次出现的 field = "值"（例如 name / pdxmesh）
std::optional<std::string> firstQuoted(const std::string& block, const std::string& field) {
    const std::regex re(field + R"re(\s*=\s*"([^"]+)")re");
    std::smatch m;
    if (std::regex_search(block, m, re)) return m[1].str();
    return std::nullopt;
}

// 生成某个颜色用的 key 前缀：
//   "lunar_capital_psi_event_{color}_diff_" -> "lunar_capital_psi_event_red_diff_AGS_psi_"
// 默认保留原始 AGS_psi_ 前缀（ship_size 里的 entity 名不会变，引擎是往它前面拼
// graphical_culture 名，拼出来的名字必须和注册的 entity 完全一致）。
std::string makeNamePrefix(const std::string& tmpl, const std::string& color, bool stripSourcePrefix) {
    std::string p = std::regex_replace(tmpl, std::regex(R"(\{color\})"), color);
    if (!stripSourcePrefix) p += kKeyPrefix;
    return p;
}

// 块文本里所有 AGS_psi_ 引用与 ags_psionic_*_diffuse.dds 一起换名/换色
std::string colorizeBlock(const std::string& block, const std::string& namePrefix, const std::string& color) {
    // 先保护 @变量引用（例如 scale = @AGS_psi_Battleship_scale）：
    // 这些是文件级脚本变量，生成文件里会原样复制其定义，不能跟着改名。
    // 注意：占位必须把 "AGS_psi_" 这段字符整体换掉，否则后面的改名正则仍会命中它。
    const std::string marker = "\x01";
    std::string out = std::regex_replace(block, std::regex("@AGS_psi_"), marker);

    // 注意："AGS_psionic_xxx.dds" 不含 "AGS_psi_"（后面紧跟 'o' 而不是 '_'），所以这条替换是安全的
    const std::regex keyRe("AGS_psi_");
    out = std::regex_replace(out, keyRe, namePrefix);

    const std::regex texRe(R"(ags_psionic_[A-Za-z0-9_]*_diffuse\.dds)");
    out = std::regex_replace(out, texRe, "ags_psionic_" + color + "_diffuse.dds");

    // 还原被保护的 @变量引用
    out = std::regex_replace(out, std::regex(marker), "@AGS_psi_");
    return out;
}

// 收集文件顶部的 @变量定义（Paradox 的 @ 变量是文件级的，生成文件必须自带一份）
std::vector<std::string> collectAtVariables(const std::string& text) {
    std::vector<std::string> vars;
    std::istringstream iss(text);
    std::string line;
    const std::regex re(R"(^\s*@[A-Za-z0-9_]+\s*=.*$)");
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (std::regex_match(line, re)) {
            // 去掉行尾 \r，保留原始写法
            if (!line.empty() && line.back() == '\r') line.pop_back();
            vars.push_back(line);
        }
    }
    return vars;
}

// 是否已经是本工具生成的变体（保证重复运行幂等）
bool isGeneratedVariant(const std::string& key, const std::set<std::string>& knownColors) {
    if (key.rfind(kGeneratedPrefix, 0) == 0) return true;   // 新命名 lunar_capital_psi_event_*
    const std::string prefix = kKeyPrefix;
    if (key.rfind(prefix, 0) != 0) return false;
    const std::string rest = key.substr(prefix.size());
    for (const auto& c : knownColors) {
        if (rest.rfind(c + "_", 0) == 0) return true;   // AGS_psi_<color>_...
    }
    return false;
}

std::set<std::string> detectTextures(const fs::path& dir) {
    std::set<std::string> colors;
    const std::regex re(R"(^ags_psionic_([A-Za-z0-9]+)_diffuse\.dds$)");
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        std::smatch m;
        if (std::regex_match(name, m, re)) colors.insert(m[1].str());
    }
    return colors;
}

struct Stats {
    int meshes = 0;
    int entities = 0;
    int skipped = 0;
};

// 处理一个定义文件：blockKey = "pdxmesh" 或 "entity"
// wrapInObjectTypes：.gfx 需要临时包进 objectTypes = { } 里
std::string processFile(const fs::path& path,
                        const std::string& blockKey,
                        bool wrapInObjectTypes,
                        const std::vector<std::string>& colors,
                        const std::string& namePrefixTmpl,
                        bool stripSourcePrefix,
                        std::vector<std::string>& keyReport,
                        Stats& stats) {
    const std::string text = readFile(path);
    const auto blocks = findBlocks(text, blockKey);

    // 先收集所有"待变体化"的原块
    struct Item { std::string key; std::string body; };
    std::vector<Item> items;
    std::set<std::string> knownColors(colors.begin(), colors.end());
    knownColors.insert(kDefaultColor);

    for (const auto& [b, e] : blocks) {
        const std::string body = text.substr(b, e - b);
        const auto key = firstQuoted(body, "name");
        if (!key) continue;
        if (key->rfind(kKeyPrefix, 0) != 0) { ++stats.skipped; continue; }  // 非灵能键（如 lunar_capital_*）
        if (isGeneratedVariant(*key, knownColors)) { ++stats.skipped; continue; } // 已经是变体（保证幂等）
        items.push_back({*key, body});
    }

    std::ostringstream out;
    out << "# 由 utils/ships/ags_psionic_variants.cpp 自动生成，请勿手改。" << "\n";
    out << "# 源文件: " << path.filename().string() << "\n";
    out << "# 默认色: " << kDefaultColor << "（在主定义文件中注册，不在此重复）\n";
    out << "# key 命名: " << namePrefixTmpl << " + 原始 key"
        << (stripSourcePrefix ? "（已去掉 AGS_psi_ 前缀）" : "") << "\n";
    out << "# 生成颜色:";
    for (const auto& c : colors) out << " " << c;
    out << "\n\n";

    // @变量定义原样复制（@ 变量是文件级的，不复制会导致 scale 等引用失效）
    const auto atVars = collectAtVariables(text);
    if (!atVars.empty()) {
        out << "# --- 原样复制源文件的 @ 变量定义 ---\n";
        for (const auto& v : atVars) out << v << "\n";
        out << "\n";
    }

    for (const auto& color : colors) {
        if (wrapInObjectTypes) out << "objectTypes = {\n";
        const std::string namePrefix = makeNamePrefix(namePrefixTmpl, color, stripSourcePrefix);
        for (const auto& item : items) {
            const std::string variant = colorizeBlock(item.body, namePrefix, color);
            // 缩进一层，保持可读性
            std::istringstream iss(variant);
            std::string line;
            while (std::getline(iss, line)) {
                out << (wrapInObjectTypes ? "    " : "") << line << "\n";
            }
            out << "\n";

            const auto variantKey = firstQuoted(variant, "name");
            keyReport.push_back("# " + color + ": " + item.key + "  ->  " +
                                (variantKey ? *variantKey : std::string("?")));
        }
        if (wrapInObjectTypes) out << "}\n\n";
    }

    if (blockKey == "pdxmesh") stats.meshes += static_cast<int>(items.size());
    else stats.entities += static_cast<int>(items.size());
    return out.str();
}

void printUsage(const char* argv0) {
    std::cout <<
        "用法: " << argv0 << " [选项]\n"
        "  --dir <目录>       定义文件所在目录（默认 <mod>/gfx/models/ships/AGS_Ships）\n"
        "  --out <目录>       输出目录（默认与 --dir 相同）\n"
        "  --colors a,b,c     只生成指定颜色（默认自动探测 ags_psionic_<color>_diffuse.dds）\n"
        "  --suffix <后缀>    输出文件名后缀（默认 _diff）\n"
        "  --name-prefix <模板>  key 前缀模板，{color} 替换成颜色名\n"
        "                     （默认 lunar_capital_psi_event_{color}_diff_，原始 AGS_psi_ 名整体保留）\n"
        "  --strip-source-prefix 变体 key 里去掉 AGS_psi_（默认保留）\n"
        "  --list-keys <文件> 额外导出 原名->变体名 对照表\n"
        "  --dry-run          只打印统计，不写文件\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Options opt;
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto next = [&]() -> std::string {
                if (i + 1 >= argc) throw std::runtime_error("缺少参数: " + a);
                return argv[++i];
            };
            if (a == "--dir") opt.dir = next();
            else if (a == "--out") opt.out = next();
            else if (a == "--suffix") opt.suffix = next();
            else if (a == "--name-prefix") opt.namePrefix = next();
            else if (a == "--strip-source-prefix") opt.stripSourcePrefix = true;
            else if (a == "--list-keys") opt.listKeys = next();
            else if (a == "--dry-run") opt.dryRun = true;
            else if (a == "--help" || a == "-h") { printUsage(argv[0]); return 0; }
            else if (a == "--colors") {
                std::istringstream iss(next());
                std::string c;
                while (std::getline(iss, c, ',')) {
                    if (!c.empty()) opt.colors.insert(c);
                }
            } else {
                throw std::runtime_error("未知参数: " + a);
            }
        }

        if (opt.dir.empty()) {
            // 默认：<mod 根>/gfx/models/ships/AGS_Ships
            opt.dir = fs::current_path() / "gfx/models/ships/AGS_Ships";
        }
        if (!fs::is_directory(opt.dir)) {
            throw std::runtime_error("目录不存在: " + opt.dir.string() +
                                     "（请用 --dir 指定 AGS_Ships 目录）");
        }
        if (opt.out.empty()) opt.out = opt.dir;

        // 颜色列表
        const auto available = detectTextures(opt.dir);
        std::vector<std::string> colors;
        if (opt.colors.empty()) {
            for (const auto& c : available) {
                if (c != kDefaultColor) colors.push_back(c);
            }
        } else {
            for (const auto& c : opt.colors) colors.push_back(c);
        }
        if (colors.empty()) {
            std::cout << "没有找到任何非默认颜色的贴图（ags_psionic_<color>_diffuse.dds），无事可做。\n";
            return 0;
        }

        std::cout << "AGS_Ships 目录 : " << opt.dir.string() << "\n"
                  << "输出目录       : " << opt.out.string() << "\n"
                  << "key 命名模板   : " << opt.namePrefix
                  << (opt.stripSourcePrefix ? "（变体名去掉 AGS_psi_）" : "（保留原始 AGS_psi_ 名）") << "\n"
                  << "已发现贴图颜色 :";
        for (const auto& c : available) std::cout << " " << c;
        std::cout << "\n本次生成颜色   :";
        for (const auto& c : colors) std::cout << " " << c;
        std::cout << "\n";
        std::cout << "对应的 graphical_culture 名（每个颜色一个）:\n";
        for (const auto& c : colors) {
            std::cout << "  " << std::regex_replace(opt.namePrefix, std::regex(R"(\{color\})"), c) << "\n";
        }
        std::cout << "\n";

        // 缺贴图告警
        for (const auto& c : colors) {
            const fs::path tex = opt.dir / ("ags_psionic_" + c + "_diffuse.dds");
            if (!fs::exists(tex)) {
                std::cout << "[警告] 缺少贴图: " << tex.filename().string()
                          << "（仍会注册引用，请自行补上该 dds）\n";
            }
        }

        // 收集要处理的定义文件：_spth_psi_ags*.gfx / *.asset
        std::vector<fs::path> gfxFiles, assetFiles;
        for (const auto& entry : fs::directory_iterator(opt.dir)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (name.rfind("_spth_psi_ags", 0) != 0) continue;      // 只处理这些定义文件
            const std::string stem = entry.path().stem().string();
            if (stem.size() >= opt.suffix.size() &&
                stem.compare(stem.size() - opt.suffix.size(), opt.suffix.size(), opt.suffix) == 0) {
                std::cout << "跳过本工具的输出文件: " << name << "\n";   // 避免把自己的 _diff 输出当输入
                continue;
            }
            if (entry.path().extension() == ".gfx") gfxFiles.push_back(entry.path());
            else if (entry.path().extension() == ".asset") assetFiles.push_back(entry.path());
        }
        std::sort(gfxFiles.begin(), gfxFiles.end());
        std::sort(assetFiles.begin(), assetFiles.end());

        if (gfxFiles.empty() && assetFiles.empty()) {
            throw std::runtime_error("在目录里没找到 _spth_psi_ags*.gfx / *.asset");
        }

        Stats stats;
        std::vector<std::string> keyReport;
        int written = 0;

        auto handle = [&](const fs::path& src, const std::string& blockKey, bool wrap) {
            const std::string content = processFile(src, blockKey, wrap, colors,
                                                   opt.namePrefix, opt.stripSourcePrefix, keyReport, stats);
            const fs::path dst = opt.out / (src.stem().string() + opt.suffix + src.extension().string());
            std::cout << (opt.dryRun ? "[dry-run] 将写入 " : "写入 ") << dst.filename().string()
                      << "  (" << (blockKey == "pdxmesh" ? "mesh" : "entity") << " x "
                      << colors.size() << " 色)\n";
            if (!opt.dryRun) { writeFile(dst, content); ++written; }
        };

        for (const auto& f : gfxFiles) handle(f, "pdxmesh", true);
        for (const auto& f : assetFiles) handle(f, "entity", false);

        std::cout << "\n完成：mesh " << stats.meshes << " 个 × " << colors.size() << " 色，"
                  << "entity " << stats.entities << " 个 × " << colors.size() << " 色"
                  << "（跳过非灵能/已是变体的块 " << stats.skipped << " 个）\n";
        if (!opt.dryRun) std::cout << "已写出 " << written << " 个文件到 " << opt.out.string() << "\n";

        if (!opt.listKeys.empty()) {
            std::ostringstream rep;
            rep << "# AGS 灵能舰船变体 key 对照表（由 ags_psionic_variants.cpp 生成）\n";
            rep << "# 默认色 " << kDefaultColor << " 使用原名，不在下表内\n";
            rep << "# 命名模板: " << opt.namePrefix << "（{color} = 颜色名）\n";
            if (opt.stripSourcePrefix) {
                rep << "# 已开启 --strip-source-prefix：变体名里不含 AGS_psi_ 前缀\n";
            } else {
                rep << "# 原始 AGS_psi_* 名字整体保留（变体名 = 模板 + 原始名）\n";
            }
            rep << "#\n";
            rep << "# 若要按 graphical_culture 换色，为每个颜色定义一个 graphical_culture，key 如下：\n";
            for (const auto& c : colors) {
                rep << "#   " << std::regex_replace(opt.namePrefix, std::regex(R"(\{color\})"), c) << "\n";
            }
            rep << "# 引擎查实体名的方式是 <culture 名> + \"_\" + ship_size 里写的 entity 名\n";
            rep << "# （CGfxCulture::GetEntity / GetAssetName，stellaris_4.5_source.cpp:686731），\n";
            rep << "# 因此上表必须与这里生成的 key 完全对应。\n\n";
            for (const auto& line : keyReport) rep << line << "\n";
            writeFile(opt.listKeys, rep.str());
            std::cout << "key 对照表已写入 " << opt.listKeys.string() << "\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
