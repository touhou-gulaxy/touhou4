// =============================================================================
//  tech_dependency_analyzer.cpp
//
//  用途:
//    解析 Stellaris 科技定义文件 (common/technology/*.txt), 提取顶层块
//    ("tech_xxx = { ... }"), 以及每个科技块中的 prerequisites / potential,
//    把它们存成结构体, 分析科技之间的依赖关系, 并生成若干输出文件。
//
//  输出 (默认写入 --out-dir):
//    1. all_techs.lst
//         所有科技名, 逐行输出 (按文件 / 文件内出现顺序)。
//    2. tech_give_effects.txt
//         按依赖顺序 (被依赖的科技在前) 逐行输出 effect:
//           * 默认: give_technology = { tech = <name> message = yes }
//           * potential 含 is_ai = yes 的科技默认排除; 只有当它被列表内的
//             科技 (非 is_ai = yes 的科技) 依赖时才补回 (--keep-ai-only
//             可完全关闭该排除)。
//           * levels = -1 的科技不加入 (无限重复科技)。
//           * levels 在 [1,100] 的科技用 (while 循环给予):
//                 while = {
//                     count = <levels>
//                     give_technology = { tech = <name> message = yes }
//                 }
//           * potential 含 spth_has_ag_mod = yes 的科技用 if 包裹
//             (外层 if, 内层 while):
//                 if = {
//                     limit = { spth_has_ag_mod = yes }
//                     ...
//                 }
//           * potential 含 always = no 的科技不加入, 见输出 5。
//    3. tech_has_country_flag_analysis.txt
//         分析 "含有 has_country_flag, 但没有 has_country_flag =
//         touhou_debug_all_tech_flag" 的科技。
//    4. tech_ai_only_potential.txt
//         potential 中出现 is_ai = yes 的科技及其 potential 块
//         (这些科技已从 tech_give_effects.txt 中排除)。
//    5. tech_always_no_potential.txt
//         potential 中出现 always = no 的科技及其 potential 块
//         (这些科技已从 tech_give_effects.txt 中排除)。
//    6. tech_dependencies.tsv
//         依赖关系明细 (TSV), 方便用表格 / 脚本继续处理。
//
//  编译:
//     g++ -std=c++17 -O2 -o tech_dependency_analyzer tech_dependency_analyzer.cpp
//
//  运行:
//     ./tech_dependency_analyzer
//     ./tech_dependency_analyzer --tech-dir <目录> --out-dir <目录>
//                                [--include-flag-deps] [--flag-scope potential|block]
//
//  说明:
//    * 只遍历 --tech-dir 顶层的 *.txt, 不会递归进入 category 等子目录。
//    * 文件名中包含 "placeholder" 的文件会被跳过 (可用 --no-skip-placeholder
//      取消该行为)。
//    * 注释 (# 到行尾) 会被忽略, 因此被注释掉的 prerequisites 不会被当成依赖。
// =============================================================================

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// 词法分析
// -----------------------------------------------------------------------------

struct Token {
    enum class Kind { Word, String, Symbol };
    Kind kind;
    std::string text;
    int line = 1;
};

static std::vector<Token> lex(const std::string& src) {
    std::vector<Token> out;
    size_t i = 0;
    int line = 1;

    auto isStop = [](char c) {
        return c == '#' || c == '"' || c == '{' || c == '}' || c == '=' ||
               c == '<' || c == '>' || c == '!' || c == '?';
    };

    while (i < src.size()) {
        char c = src[i];
        if (c == '\n') { ++line; ++i; continue; }
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        if (c == '#') { while (i < src.size() && src[i] != '\n') ++i; continue; }

        if (c == '"') {
            ++i;
            std::string s;
            while (i < src.size() && src[i] != '"') {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    s.push_back(src[i + 1]);
                    i += 2;
                    continue;
                }
                if (src[i] == '\n') ++line;
                s.push_back(src[i]);
                ++i;
            }
            if (i < src.size()) ++i;  // 收尾引号
            out.push_back({Token::Kind::String, s, line});
            continue;
        }

        if (c == '{' || c == '}') {
            out.push_back({Token::Kind::Symbol, std::string(1, c), line});
            ++i;
            continue;
        }

        // 比较 / 赋值运算符 (>, <, >=, <=, !=, ==, ?=)
        if (c == '=' || c == '<' || c == '>' || c == '!' || c == '?') {
            std::string op(1, c);
            if (i + 1 < src.size()) {
                char n = src[i + 1];
                bool twoChar = (c == '=' && n == '=') || (c == '<' && (n == '=' || n == '>')) ||
                               (c == '>' && n == '=') || (c == '!' && n == '=') ||
                               (c == '?' && n == '=');
                if (twoChar) {
                    op.push_back(n);
                    i += 2;
                    out.push_back({Token::Kind::Symbol, op, line});
                    continue;
                }
            }
            ++i;
            out.push_back({Token::Kind::Symbol, op, line});
            continue;
        }

        // 普通单词: 标识符 / 数字 / @变量 / 负数等
        std::string w;
        while (i < src.size()) {
            char d = src[i];
            if (std::isspace(static_cast<unsigned char>(d)) || isStop(d)) break;
            if (d == '-' && w.empty() && i + 1 < src.size() &&
                std::isdigit(static_cast<unsigned char>(src[i + 1]))) {
                w.push_back(d);
                ++i;
                continue;
            }
            w.push_back(d);
            ++i;
        }
        out.push_back({Token::Kind::Word, w, line});
    }
    return out;
}

// -----------------------------------------------------------------------------
// 语法树
//   node.key  : 键名; 空表示 "裸值" (列表项) 或匿名块
//   node.op   : "=", ">", "<" 等; 裸值节点为空
//   node.value: 标量值 (仅 !isBlock 时有效)
// -----------------------------------------------------------------------------

struct Node {
    std::string key;
    std::string op;
    std::string value;
    bool quoted = false;   // 值是否来自字符串字面量
    bool isBlock = false;
    int line = 0;
    std::vector<Node> kids;
};

static bool isOperator(const std::string& s) {
    return s == "=" || s == "<" || s == ">" || s == "<=" || s == ">=" ||
           s == "!=" || s == "==" || s == "?=";
}

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : t_(toks) {}

    std::vector<Node> parse() { return parseBody(false); }

private:
    const std::vector<Token>& t_;
    size_t i_ = 0;

    bool eof() const { return i_ >= t_.size(); }
    const Token& peek() const { return t_[i_]; }
    bool isSym(const std::string& s) const {
        return !eof() && t_[i_].kind == Token::Kind::Symbol && t_[i_].text == s;
    }

    std::vector<Node> parseBody(bool nested) {
        std::vector<Node> out;
        while (!eof()) {
            if (isSym("}")) {
                ++i_;
                if (nested) return out;
                continue;  // 多余的右括号, 忽略
            }
            if (isSym("{")) {  // 匿名块
                Node n;
                n.isBlock = true;
                n.line = peek().line;
                ++i_;
                n.kids = parseBody(true);
                out.push_back(std::move(n));
                continue;
            }

            Token keyTok = peek();
            ++i_;

            Node n;
            n.line = keyTok.line;

            if (!eof() && peek().kind == Token::Kind::Symbol && isOperator(peek().text)) {
                n.key = keyTok.text;
                n.op = peek().text;
                ++i_;
                if (isSym("{")) {
                    n.isBlock = true;
                    ++i_;
                    n.kids = parseBody(true);
                } else if (!eof() && !isSym("}")) {
                    Token v = peek();
                    ++i_;
                    n.value = v.text;
                    n.quoted = (v.kind == Token::Kind::String);
                }
                out.push_back(std::move(n));
                continue;
            }

            if (isSym("{")) {  // KEY { ... } (省略 = 的写法)
                n.key = keyTok.text;
                n.op = "=";
                n.isBlock = true;
                ++i_;
                n.kids = parseBody(true);
                out.push_back(std::move(n));
                continue;
            }

            // 裸值 / 列表项, 例如 category = { spirit_power } 里的 spirit_power
            n.value = keyTok.text;
            n.quoted = (keyTok.kind == Token::Kind::String);
            out.push_back(std::move(n));
        }
        return out;
    }
};

// -----------------------------------------------------------------------------
// 结构体: 科技定义
// -----------------------------------------------------------------------------

struct FlagCond {
    std::string flag;
    bool negated = false;  // 位于 NOT / NOR 之下
    int line = 0;
};

struct Tech {
    std::string name;
    std::string file;             // 所在文件名 (不含路径, 便于阅读)
    int line = 0;                 // 块起始行号
    size_t order = 0;             // 全局出现顺序

    std::vector<std::string> prerequisites;      // prerequisites 中列出的名字
    std::vector<std::string> prereqVariables;    // @开头 / 无法解析的写法

    bool hasPotential = false;
    std::string potentialText;                   // 还原成文本的 potential 块
    std::string potentialInnerText;              // 只剩 potential 内部内容, 便于报告缩进
    std::vector<FlagCond> flagsInPotential;      // potential 里的 has_country_flag
    std::vector<FlagCond> flagsInBlock;          // 整个科技块里的 has_country_flag

    bool aiOnly = false;                         // potential 中出现 is_ai = yes
    bool alwaysNoInPotential = false;            // potential 中出现 always = no (未被注释)
    bool agModInPotential = false;               // potential 中出现 spth_has_ag_mod = yes

    bool hasLevels = false;                      // 是否有 levels 字段
    long long levels = 0;                        // levels 的数值
    bool levelsParsed = true;                    // levels 是否是纯数字

    bool inGiveEffects = false;                  // 最终是否进入 tech_give_effects.txt

    size_t index = 0;                            // 在 techs 数组中的下标
    std::vector<size_t> deps;                    // 依赖的科技 (prerequisites 中的内部科技)
    std::vector<size_t> flagDeps;                // 由 has_country_flag = <科技名> 推断出的依赖
};

// -----------------------------------------------------------------------------
// 辅助函数
// -----------------------------------------------------------------------------

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// 把语法树节点还原成缩进文本
static void renderNode(const Node& n, const std::string& padIn,
                       const std::string& unit, std::string& out) {
    std::string pad = padIn;
    if (n.isBlock) {
        if (n.key.empty()) {
            out += pad + "{\n";
        } else {
            out += pad + n.key + " = {\n";
        }
        for (const Node& k : n.kids) renderNode(k, pad + unit, unit, out);
        out += pad + "}\n";
        return;
    }
    if (n.key.empty()) {
        out += pad + n.value + "\n";
        return;
    }
    out += pad + n.key + " " + n.op + " " + n.value + "\n";
}

static std::string render(const Node& n) {
    std::string out;
    renderNode(n, "", "\t", out);
    return out;
}

// 只渲染块的内容 (不含外层 "{ }"), 便于在报告里做缩进
static std::string renderChildren(const Node& n) {
    std::string out;
    for (const Node& k : n.kids) renderNode(k, "", "    ", out);
    return out;
}

// 收集块内所有标量值 (列表项 + 简单键值)
static void collectScalars(const Node& n, std::vector<std::string>& out) {
    if (!n.isBlock) {
        if (!n.value.empty()) out.push_back(n.value);
        return;
    }
    for (const Node& k : n.kids) collectScalars(k, out);
}

// 收集 has_country_flag 条件
static void collectFlags(const Node& n, bool negated, std::vector<FlagCond>& out) {
    if (!n.isBlock) {
        if (n.key == "has_country_flag" && !n.value.empty()) {
            out.push_back({n.value, negated, n.line});
        }
        return;
    }
    bool childNegated = negated || n.key == "NOT" || n.key == "NOR";
    for (const Node& k : n.kids) collectFlags(k, childNegated, out);
}

static const Node* findChild(const Node& block, const std::string& key) {
    for (const Node& k : block.kids) {
        if (k.key == key) return &k;
    }
    return nullptr;
}

// 收集块内所有 is_ai 的取值
static void collectIsAi(const Node& n, std::vector<std::string>& out) {
    if (!n.isBlock) {
        if (n.key == "is_ai" && !n.value.empty()) out.push_back(n.value);
        return;
    }
    for (const Node& k : n.kids) collectIsAi(k, out);
}

// 收集块内某个键的所有标量取值
static void collectKeyValues(const Node& n, const std::string& key,
                             std::vector<std::string>& out) {
    if (!n.isBlock) {
        if (n.key == key && !n.value.empty()) out.push_back(n.value);
        return;
    }
    for (const Node& k : n.kids) collectKeyValues(k, key, out);
}

// 解析整数 (允许负号); 成功返回 true
static bool parseInt(const std::string& s, long long& out) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i >= s.size()) return false;
    for (size_t j = i; j < s.size(); ++j) {
        if (!std::isdigit(static_cast<unsigned char>(s[j]))) return false;
    }
    try {
        out = std::stoll(s);
    } catch (...) {
        return false;
    }
    return true;
}

static std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

// -----------------------------------------------------------------------------
// 命令行 = 配置
// -----------------------------------------------------------------------------

struct Options {
    fs::path techDir = "/home/koishi/IdeaProjects/touhou4/common/technology";
    fs::path outDir = "/home/koishi/IdeaProjects/touhou4/utils/tech";
    bool skipPlaceholder = true;
    bool includeFlagDeps = false;      // 是否把 potential 中 has_country_flag = <科技名> 也算作依赖
    bool excludeAiOnly = true;         // 从 effect 输出中排除 potential 含 is_ai = yes 的科技
    // 分析文件扫描范围: potential 块 / 整个科技块
    bool flagScopePotentialOnly = true;
    std::string debugFlag = "touhou_debug_all_tech_flag";
};

static void printUsage(const char* argv0) {
    std::cout <<
        "用法: " << argv0 << " [选项]\n"
        "  --tech-dir <dir>            科技定义目录 (默认 " << Options{}.techDir.string() << ")\n"
        "  --out-dir <dir>             输出目录 (默认 " << Options{}.outDir.string() << ")\n"
        "  --no-skip-placeholder       不跳过文件名含 placeholder 的文件\n"
        "  --include-flag-deps         potential 中 has_country_flag = <科技名> 也视为依赖\n"
        "  --keep-ai-only              不排除 potential 含 is_ai = yes 的科技\n"
        "  --flag-scope <potential|block>  分析文件扫描范围 (默认 potential)\n"
        "  --debug-flag <name>         调试用全局科技旗标 (默认 touhou_debug_all_tech_flag)\n"
        "  -h, --help                  显示帮助\n";
}

// -----------------------------------------------------------------------------
// 主流程
// -----------------------------------------------------------------------------

struct TopoResult {
    std::vector<size_t> order;              // 拓扑序: 被依赖的科技在前
    std::vector<std::string> cycleMembers;  // 处于环中的科技 (无法完全满足依赖顺序)
};

// 拓扑排序 (Kahn), 同层按出现顺序稳定输出; 环内节点用 DFS 后序兜底
static TopoResult topoSort(const std::vector<Tech>& techs, bool includeFlagDeps) {
    TopoResult res;
    std::vector<std::vector<size_t>> dependents(techs.size());
    std::vector<int> indegree(techs.size(), 0);

    for (const Tech& t : techs) {
        std::vector<size_t> edges = t.deps;
        if (includeFlagDeps) edges.insert(edges.end(), t.flagDeps.begin(), t.flagDeps.end());
        std::sort(edges.begin(), edges.end());
        edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
        for (size_t d : edges) {
            dependents[d].push_back(t.index);
            indegree[t.index]++;
        }
    }

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> ready;
    for (const Tech& t : techs) {
        if (indegree[t.index] == 0) ready.push(t.index);
    }

    std::vector<bool> emitted(techs.size(), false);
    res.order.reserve(techs.size());
    while (!ready.empty()) {
        size_t i = ready.top();
        ready.pop();
        if (emitted[i]) continue;
        emitted[i] = true;
        res.order.push_back(i);
        for (size_t d : dependents[i]) {
            if (--indegree[d] == 0) ready.push(d);
        }
    }

    if (res.order.size() != techs.size()) {
        std::function<void(size_t)> dfs = [&](size_t i) {
            if (emitted[i]) return;
            emitted[i] = true;
            for (size_t d : techs[i].deps) dfs(d);
            if (includeFlagDeps) {
                for (size_t d : techs[i].flagDeps) dfs(d);
            }
            res.order.push_back(i);
        };
        for (size_t i = 0; i < techs.size(); ++i) {
            if (!emitted[i]) {
                if (indegree[i] > 0) res.cycleMembers.push_back(techs[i].name);
                dfs(i);
            }
        }
    }
    return res;
}

int main(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "参数 " << what << " 缺少值\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { printUsage(argv[0]); return 0; }
        else if (a == "--tech-dir") opt.techDir = next("--tech-dir");
        else if (a == "--out-dir") opt.outDir = next("--out-dir");
        else if (a == "--no-skip-placeholder") opt.skipPlaceholder = false;
        else if (a == "--include-flag-deps") opt.includeFlagDeps = true;
        else if (a == "--keep-ai-only") opt.excludeAiOnly = false;
        else if (a == "--flag-scope") {
            std::string v = next("--flag-scope");
            if (v != "potential" && v != "block") {
                std::cerr << "--flag-scope 只能是 potential 或 block\n";
                return 2;
            }
            opt.flagScopePotentialOnly = (v == "potential");
        }
        else if (a == "--debug-flag") opt.debugFlag = next("--debug-flag");
        else {
            std::cerr << "未知参数: " << a << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    if (!fs::is_directory(opt.techDir)) {
        std::cerr << "目录不存在: " << opt.techDir << "\n";
        return 1;
    }
    std::error_code ec;
    fs::create_directories(opt.outDir, ec);
    if (ec) {
        std::cerr << "无法创建输出目录 " << opt.outDir << ": " << ec.message() << "\n";
        return 1;
    }

    // ---- 1. 收集顶层文件 (不递归) -------------------------------------------
    std::vector<fs::path> files;
    for (const fs::directory_entry& e : fs::directory_iterator(opt.techDir)) {
        if (!e.is_regular_file()) continue;
        fs::path p = e.path();
        if (toLower(p.extension().string()) != ".txt") continue;
        std::string fname = p.filename().string();
        if (opt.skipPlaceholder && toLower(fname).find("placeholder") != std::string::npos) {
            std::cout << "[跳过] " << fname << " (文件名含 placeholder)\n";
            continue;
        }
        files.push_back(p);
    }
    std::sort(files.begin(), files.end());

    // ---- 2. 解析, 抽取顶层科技块 -------------------------------------------
    std::vector<Tech> techs;
    std::unordered_map<std::string, size_t> byName;
    std::vector<std::string> duplicateTechs;

    for (const fs::path& path : files) {
        std::string src = readFile(path);
        std::vector<Token> toks = lex(src);
        Parser parser(toks);
        std::vector<Node> top = parser.parse();

        for (const Node& n : top) {
            if (!n.isBlock) continue;                        // @变量 / 标量, 跳过
            if (n.key.rfind("tech_", 0) != 0) continue;      // 只关心 tech_ 开头的块

            Tech t;
            t.name = n.key;
            t.file = path.filename().string();
            t.line = n.line;
            t.index = techs.size();
            t.order = techs.size();

            // prerequisites
            for (const Node& k : n.kids) {
                if (k.key != "prerequisites") continue;
                std::vector<std::string> vals;
                collectScalars(k, vals);
                for (std::string& v : vals) {
                    if (v.empty()) continue;
                    if (v[0] == '@') t.prereqVariables.push_back(v);
                    else t.prerequisites.push_back(v);
                }
            }

            // levels
            for (const Node& k : n.kids) {
                if (k.key != "levels" || k.isBlock) continue;
                t.hasLevels = true;
                t.levelsParsed = parseInt(k.value, t.levels);
            }

            // potential
            if (const Node* pot = findChild(n, "potential")) {
                t.hasPotential = true;
                t.potentialText = render(*pot);
                t.potentialInnerText = renderChildren(*pot);
                collectFlags(*pot, false, t.flagsInPotential);
                std::vector<std::string> aiVals;
                collectIsAi(*pot, aiVals);
                for (const std::string& v : aiVals) {
                    if (toLower(v) == "yes") t.aiOnly = true;
                }
                std::vector<std::string> alwaysVals;
                collectKeyValues(*pot, "always", alwaysVals);
                for (const std::string& v : alwaysVals) {
                    if (toLower(v) == "no") t.alwaysNoInPotential = true;
                }
                std::vector<std::string> agVals;
                collectKeyValues(*pot, "spth_has_ag_mod", agVals);
                for (const std::string& v : agVals) {
                    if (toLower(v) == "yes") t.agModInPotential = true;
                }
            }
            // 整个块内的 has_country_flag
            collectFlags(n, false, t.flagsInBlock);

            if (byName.count(t.name)) duplicateTechs.push_back(t.name);
            byName[t.name] = t.index;
            techs.push_back(std::move(t));
        }
    }

    if (!duplicateTechs.empty()) {
        std::cerr << "[警告] 重复定义的科技: " << join(duplicateTechs, ", ") << "\n";
    }

    // ---- 3. 建立依赖图 -----------------------------------------------------
    std::vector<std::string> externalPrereqs;
    std::set<std::string> externalSet;

    for (Tech& t : techs) {
        std::set<size_t> seen;
        for (const std::string& p : t.prerequisites) {
            auto it = byName.find(p);
            if (it == byName.end()) {
                if (externalSet.insert(p).second) externalPrereqs.push_back(p);
                continue;
            }
            if (it->second == t.index) continue;  // 自引用忽略
            if (seen.insert(it->second).second) t.deps.push_back(it->second);
        }

        // potential / weight_modifier 里出现的 has_country_flag = <其他科技名>
        std::set<size_t> seenFlag;
        for (const FlagCond& f : t.flagsInBlock) {
            if (f.negated) continue;
            auto it = byName.find(f.flag);
            if (it == byName.end() || it->second == t.index) continue;
            if (seenFlag.insert(it->second).second) t.flagDeps.push_back(it->second);
        }
    }

    // ---- 4. 拓扑排序: 被依赖的科技在前 -------------------------------------
    TopoResult topo = topoSort(techs, opt.includeFlagDeps);
    std::vector<size_t>& topoOrder = topo.order;

    // 顺便算一份 "把 has_country_flag = <科技名> 也算作依赖" 的顺序, 用于报告对比
    TopoResult topoWithFlags = topoSort(techs, true);

    // potential 中含 is_ai = yes 的科技 (默认从 effect 输出里排除)
    std::vector<size_t> aiOnlyTechs;
    for (const Tech& t : techs) {
        if (t.aiOnly) aiOnlyTechs.push_back(t.index);
    }
    // potential 中含 always = no 的科技 (永远不会出现, 单独列文件, 不给 effect)
    std::vector<size_t> alwaysNoTechs;
    for (const Tech& t : techs) {
        if (t.alwaysNoInPotential) alwaysNoTechs.push_back(t.index);
    }

    // ---- 4.1 决定哪些科技进入 effect 列表 ---------------------------------
    // levels = -1 表示无限重复科技, 不加入
    std::vector<size_t> infiniteTechs;
    for (const Tech& t : techs) {
        if (t.hasLevels && t.levels < 0) infiniteTechs.push_back(t.index);
    }

    // 依赖方向: 每个科技在本目录内的前置 (含可选的 flag 推断依赖)
    auto edgesOf = [&](const Tech& t) {
        std::vector<size_t> e = t.deps;
        if (opt.includeFlagDeps) e.insert(e.end(), t.flagDeps.begin(), t.flagDeps.end());
        std::sort(e.begin(), e.end());
        e.erase(std::unique(e.begin(), e.end()), e.end());
        return e;
    };
    std::vector<std::vector<size_t>> dependentsOf(techs.size());
    for (const Tech& t : techs) {
        for (size_t d : edgesOf(t)) dependentsOf[d].push_back(t.index);
    }

    // 基础名单: 非 ai 专用 / 非 always = no / 非 levels < 0
    std::vector<char> base(techs.size(), 0);
    for (const Tech& t : techs) {
        if (opt.excludeAiOnly && t.aiOnly) continue;
        if (t.alwaysNoInPotential) continue;
        if (t.hasLevels && t.levels < 0) continue;
        base[t.index] = 1;
    }

    // 不动点迭代:
    //   (1) 补回: ai 专用科技只在其被"当前名单内"的科技依赖时才加入;
    //   (2) 剪枝: 名单内科技若有本目录内的前置不在名单里, 说明这条路径
    //       实际走不到 (前置永远不会被给予), 该科技也一并去掉。
    // 补回与剪枝互相影响 (例如 tech_hm_pop_growth_1 只因 ai 科技才可达),
    // 所以迭代到稳定为止。
    std::vector<char> included = base;
    for (size_t iter = 0; iter <= techs.size() + 2; ++iter) {
        std::vector<char> next = base;
        if (opt.excludeAiOnly) {
            for (size_t x : aiOnlyTechs) {
                for (size_t y : dependentsOf[x]) {
                    if (!included[y]) continue;  // 依赖者不在上一轮名单内 -> 不补
                    next[x] = 1;
                    break;
                }
            }
        }
        for (const Tech& t : techs) {
            if (!next[t.index]) continue;
            for (size_t d : edgesOf(t)) {
                if (!next[d]) {
                    next[t.index] = 0;  // 前置走不到 -> 自己也不给
                    break;
                }
            }
        }
        if (next == included) break;
        included = next;
    }

    // 被补回的 ai 科技 (及触发它补回的依赖者)
    std::vector<std::pair<size_t, size_t>> addedBack;
    for (size_t x : aiOnlyTechs) {
        if (!included[x] || base[x]) continue;
        for (size_t y : dependentsOf[x]) {
            if (included[y]) {
                addedBack.emplace_back(x, y);
                break;
            }
        }
    }

    // 每个科技为什么没进 effect 名单
    std::vector<std::string> excludedReason(techs.size());
    std::vector<std::vector<std::string>> unreachablePrereq(techs.size());
    for (const Tech& t : techs) {
        if (included[t.index]) continue;
        if (t.alwaysNoInPotential) {
            excludedReason[t.index] = "potential_always_no";
        } else if (t.hasLevels && t.levels < 0) {
            excludedReason[t.index] = "levels_lt_0";
        } else if (t.aiOnly && opt.excludeAiOnly) {
            excludedReason[t.index] = "ai_only";
        } else {
            excludedReason[t.index] = "unreachable_prereq";
        }
        for (size_t d : edgesOf(t)) {
            if (!included[d]) unreachablePrereq[t.index].push_back(techs[d].name);
        }
    }

    for (Tech& t : techs) t.inGiveEffects = included[t.index] != 0;

    // 因前置不可达而没进名单的科技
    std::vector<std::string> unreachableList;
    for (const Tech& t : techs) {
        if (excludedReason[t.index] == "unreachable_prereq") {
            unreachableList.push_back(t.name + " (缺 " + join(unreachablePrereq[t.index], " ") + ")");
        }
    }

    size_t effectCount = 0;
    for (size_t i : topoOrder) {
        if (!included[i]) continue;
        ++effectCount;
    }
    // ---- 5. 输出文件 -------------------------------------------------------
    // 5.1 all_techs.lst
    {
        std::ofstream out(opt.outDir / "all_techs.lst");
        for (const Tech& t : techs) out << t.name << "\n";
    }

    // 5.2 tech_give_effects.txt
    {
        std::ofstream out(opt.outDir / "tech_give_effects.txt");
        size_t wrappedWhile = 0, wrappedIf = 0;
        for (size_t i : topoOrder) {
            if (!included[i]) continue;
            const Tech& t = techs[i];
            if (t.hasLevels && t.levels >= 1 && t.levels <= 100) ++wrappedWhile;
            if (t.agModInPotential) ++wrappedIf;
        }

        out << "# 由 tech_dependency_analyzer.cpp 生成\n"
            << "# 依赖顺序: prerequisites 中依赖的科技排在前面"
            << (opt.includeFlagDeps ? ", 并包含 has_country_flag 推断的依赖" : "")
            << "\n"
            << "# 共 " << effectCount << " 条 effect";
        if (opt.excludeAiOnly && !aiOnlyTechs.empty()) {
            out << "\n#   - is_ai = yes: 共 " << aiOnlyTechs.size() << " 个, 未加入 "
                << (aiOnlyTechs.size() - addedBack.size()) << " 个, 补回 " << addedBack.size()
                << " 个 (仅因被名单内科技依赖)";
        }
        if (!alwaysNoTechs.empty()) {
            out << "\n#   - always = no: 未加入 " << alwaysNoTechs.size()
                << " 个 (见 tech_always_no_potential.txt)";
        }
        if (!infiniteTechs.empty()) {
            out << "\n#   - levels < 0: 未加入 " << infiniteTechs.size() << " 个";
        }
        if (!unreachableList.empty()) {
            out << "\n#   - 前置不可达(实际走不到): 未加入 " << unreachableList.size() << " 个";
            for (const std::string& s : unreachableList) out << "\n#       " << s;
        }
        if (wrappedWhile) {
            out << "\n#   - levels 在 [1,100]: " << wrappedWhile << " 个用 while = { count = ... } 给予";
        }
        if (wrappedIf) {
            out << "\n#   - spth_has_ag_mod = yes: " << wrappedIf
                << " 个用 if = { limit = { spth_has_ag_mod = yes } ... } 包裹";
        }
        out << "\n";
        for (size_t i : topoOrder) {
            if (!included[i]) continue;
            const Tech& t = techs[i];
            const bool wrapWhile = t.hasLevels && t.levels >= 1 && t.levels <= 100;

            if (t.agModInPotential) {
                out << "if = {\n"
                    << "\tlimit = { spth_has_ag_mod = yes }\n";
                if (wrapWhile) {
                    out << "\twhile = {\n"
                        << "\t\tcount = " << t.levels << "\n"
                        << "\t\tgive_technology = { tech = " << t.name << " message = yes }\n"
                        << "\t}\n";
                } else {
                    out << "\tgive_technology = { tech = " << t.name << " message = yes }\n";
                }
                out << "}\n";
            } else if (wrapWhile) {
                out << "while = {\n"
                    << "\tcount = " << t.levels << "\n"
                    << "\tgive_technology = { tech = " << t.name << " message = yes }\n"
                    << "}\n";
            } else {
                out << "give_technology = { tech = " << t.name << " message = yes }\n";
            }
        }
    }

    // 5.3 tech_ai_only_potential.txt
    {
        std::ofstream out(opt.outDir / "tech_ai_only_potential.txt");
        out << "# potential 块中出现 is_ai = yes 的科技\n"
            << "# 这些科技"
            << (opt.excludeAiOnly ? "默认已" : "未")
            << "从 tech_give_effects.txt 中排除";
        if (opt.excludeAiOnly) {
            out << ";\n# 只有被 effect 名单内的科技(非 is_ai = yes)依赖的那些才会补回 (见文末)";
        }
        out << "\n"
            << "# 来源目录: " << opt.techDir.string() << "\n"
            << "#\n"
            << "# 统计: 共 " << aiOnlyTechs.size() << " 个";
        if (opt.excludeAiOnly) {
            out << "; 被补回 effect 列表 " << addedBack.size() << " 个";
        }
        out << "\n"
            << "#\n"
            << "# ==== 列表 (科技名 | 所在文件:行号) ====\n";
        std::set<size_t> addedBackSet;
        for (const auto& p : addedBack) addedBackSet.insert(p.first);
        for (size_t idx : aiOnlyTechs) {
            const Tech& t = techs[idx];
            out << t.name << " | " << t.file << ":" << t.line;
            if (addedBackSet.count(idx)) out << " | 已补回 effect 列表";
            out << "\n";
        }

        out << "\n# ==== 各科技的 potential 块 ====\n";
        for (size_t idx : aiOnlyTechs) {
            const Tech& t = techs[idx];
            out << "\n# " << t.name << " (" << t.file << ":" << t.line << ")\n";
            if (t.hasPotential) {
                out << "potential = {\n" << t.potentialInnerText << "}\n";
            } else {
                out << "(无 potential 块)\n";
            }
        }

        if (opt.excludeAiOnly) {
            out << "\n# ==== 被补回 effect 列表的科技 (理由: 被其他科技依赖) ====\n";
            if (addedBack.empty()) {
                out << "(无)\n";
            } else {
                for (const auto& p : addedBack) {
                    out << techs[p.first].name << " | 被 " << techs[p.second].name
                        << " 依赖, 因此补回\n";
                }
            }
        }
    }

    // 5.4 tech_always_no_potential.txt
    {
        std::ofstream out(opt.outDir / "tech_always_no_potential.txt");
        out << "# potential 块中出现 always = no (未被注释) 的科技\n"
            << "# 这类科技永远不会被解锁, 已从 tech_give_effects.txt 中排除\n"
            << "# 来源目录: " << opt.techDir.string() << "\n"
            << "#\n"
            << "# 统计: 共 " << alwaysNoTechs.size() << " 个\n"
            << "#\n"
            << "# ==== 列表 (科技名 | 所在文件:行号) ====\n";
        for (size_t idx : alwaysNoTechs) {
            const Tech& t = techs[idx];
            out << t.name << " | " << t.file << ":" << t.line << "\n";
        }
        out << "\n# ==== 各科技的 potential 块 ====\n";
        for (size_t idx : alwaysNoTechs) {
            const Tech& t = techs[idx];
            out << "\n# " << t.name << " (" << t.file << ":" << t.line << ")\n";
            if (t.hasPotential) {
                out << "potential = {\n" << t.potentialInnerText << "}\n";
            } else {
                out << "(无 potential 块)\n";
            }
        }
    }

    // 5.5 tech_has_country_flag_analysis.txt
    {
        std::vector<size_t> hits;        // 有 has_country_flag 但没有 debug 旗标的科技
        std::vector<size_t> debugged;    // 带 debug 旗标的科技
        for (const Tech& t : techs) {
            const std::vector<FlagCond>& flags =
                opt.flagScopePotentialOnly ? t.flagsInPotential : t.flagsInBlock;
            if (flags.empty()) continue;
            bool hasDebug = false;
            for (const FlagCond& f : flags) {
                if (f.flag == opt.debugFlag) hasDebug = true;
            }
            (hasDebug ? debugged : hits).push_back(t.index);
        }

        std::ofstream out(opt.outDir / "tech_has_country_flag_analysis.txt");
        out << "# 含有 has_country_flag 但没有 has_country_flag = " << opt.debugFlag
            << " 的科技\n"
            << "# 扫描范围: "
            << (opt.flagScopePotentialOnly ? "potential 块" : "整个科技块")
            << " (可用 --flag-scope 调整)\n"
            << "# 来源目录: " << opt.techDir.string() << "\n"
            << "#\n"
            << "# 统计: 共 " << techs.size() << " 个科技; 命中 " << hits.size()
            << " 个; 其中带 " << opt.debugFlag << " 的 " << debugged.size() << " 个\n"
            << "#\n"
            << "# ==== 命中列表 (科技名 | 所在文件:行号 | 该范围里的 has_country_flag) ====\n";

        for (size_t idx : hits) {
            const Tech& t = techs[idx];
            const std::vector<FlagCond>& flags =
                opt.flagScopePotentialOnly ? t.flagsInPotential : t.flagsInBlock;
            out << t.name << " | " << t.file << ":" << t.line << " |";
            for (const FlagCond& f : flags) {
                out << " " << (f.negated ? "NOT:" : "") << f.flag;
            }
            out << "\n";
            out << "    potential:\n";
            if (t.hasPotential) {
                std::istringstream iss(t.potentialInnerText);
                std::string line;
                while (std::getline(iss, line)) out << "        " << line << "\n";
            } else {
                out << "        (无 potential 块)\n";
            }
        }

        out << "\n# ==== 带 " << opt.debugFlag << " 的科技 (未命中) ====\n";
        for (size_t idx : debugged) {
            const Tech& t = techs[idx];
            out << t.name << " | " << t.file << ":" << t.line << "\n";
        }

        // has_country_flag = <另一个科技名> 属于隐式依赖, 列出并说明是否已被 prerequisites 覆盖
        out << "\n# ==== 隐式依赖: has_country_flag 取值正好是另一个科技名 ====\n";
        size_t implicitCount = 0, uncoveredCount = 0;
        for (const Tech& t : techs) {
            for (size_t d : t.flagDeps) {
                ++implicitCount;
                bool covered = std::find(t.deps.begin(), t.deps.end(), d) != t.deps.end();
                if (!covered) ++uncoveredCount;
                out << t.name << " | " << t.file << ":" << t.line << " | 依赖 " << techs[d].name
                    << " | " << (covered ? "已被 prerequisites 覆盖" : "未被 prerequisites 覆盖")
                    << "\n";
            }
        }
        if (implicitCount == 0) out << "(无)\n";
        out << "# 合计 " << implicitCount << " 条, 其中未被 prerequisites 覆盖 "
            << uncoveredCount << " 条\n";
    }

    // 5.6 tech_dependencies.tsv
    {
        std::ofstream out(opt.outDir / "tech_dependencies.tsv");
        out << "tech\tfile\tline\tlevels\tin_give_effects\tprerequisites\t"
               "missing_prerequisites\tai_only_prereq\tflag_deps\t"
               "potential_has_country_flag\tis_ai_only\tpotential_always_no\t"
               "potential_ag_mod\texcluded_reason\tunreachable_prereq\thas_debug_flag\n";
        for (size_t i : topoOrder) {
            const Tech& t = techs[i];
            std::vector<std::string> missing;
            for (const std::string& p : t.prerequisites) {
                if (!byName.count(p)) missing.push_back(p);
            }
            // 被本程序排除出 effect 的前置科技 (目前即 is_ai = yes 的那批)
            // 前置里属于 is_ai = yes 那批的 (它们会被补回 effect 列表)
            std::vector<std::string> aiOnlyPrereq;
            for (const std::string& p : t.prerequisites) {
                auto it = byName.find(p);
                if (it != byName.end() && techs[it->second].aiOnly) aiOnlyPrereq.push_back(p);
            }
            std::vector<std::string> flagDeps;
            for (size_t d : t.flagDeps) flagDeps.push_back(techs[d].name);
            std::vector<std::string> flags;
            for (const FlagCond& f : t.flagsInPotential) {
                flags.push_back((f.negated ? "NOT:" : "") + f.flag);
            }
            bool hasDebug = false;
            for (const FlagCond& f : t.flagsInPotential) {
                if (f.flag == opt.debugFlag) hasDebug = true;
            }
            out << t.name << "\t" << t.file << "\t" << t.line << "\t"
                << (t.hasLevels ? std::to_string(t.levels) : "") << "\t"
                << (t.inGiveEffects ? "yes" : "no") << "\t"
                << join(t.prerequisites, " ") << "\t"
                << join(missing, " ") << "\t"
                << join(aiOnlyPrereq, " ") << "\t"
                << join(flagDeps, " ") << "\t"
                << join(flags, " ") << "\t"
                << (t.aiOnly ? "yes" : "no") << "\t"
                << (t.alwaysNoInPotential ? "yes" : "no") << "\t"
                << (t.agModInPotential ? "yes" : "no") << "\t"
                << excludedReason[t.index] << "\t"
                << join(unreachablePrereq[t.index], " ") << "\t"
                << (hasDebug ? "yes" : "no") << "\n";
        }
    }

    // ---- 6. 控制台摘要 -----------------------------------------------------
    std::cout << "扫描目录: " << opt.techDir << "\n"
              << "解析文件: " << files.size() << " 个\n"
              << "顶层科技: " << techs.size() << " 个\n"
              << "输出目录: " << opt.outDir << "\n"
              << "  - all_techs.lst\n"
              << "  - tech_give_effects.txt (" << effectCount << " 条)\n"
              << "  - tech_has_country_flag_analysis.txt\n"
              << "  - tech_ai_only_potential.txt (" << aiOnlyTechs.size() << " 个)\n"
              << "  - tech_always_no_potential.txt (" << alwaysNoTechs.size() << " 个)\n"
              << "  - tech_dependencies.tsv\n";

    size_t withDeps = 0;
    for (const Tech& t : techs) {
        if (!t.deps.empty()) ++withDeps;
    }
    std::cout << "有内部前置科技的科技: " << withDeps << " 个\n";
    if (!externalPrereqs.empty()) {
        std::cout << "本目录之外的前置科技 " << externalPrereqs.size() << " 个: "
                  << join(externalPrereqs, ", ") << "\n";
    }
    if (!topo.cycleMembers.empty()) {
        std::cout << "[警告] prerequisites 存在循环依赖, 已尽力断开: "
                  << join(topo.cycleMembers, ", ") << "\n";
    }
    if (opt.excludeAiOnly && !aiOnlyTechs.empty()) {
        std::cout << "potential 含 is_ai = yes 的科技 " << aiOnlyTechs.size()
                  << " 个: 未加入 " << (aiOnlyTechs.size() - addedBack.size())
                  << " 个, 补回 " << addedBack.size() << " 个\n";
    }
    if (!alwaysNoTechs.empty()) {
        std::cout << "potential 含 always = no 的科技 " << alwaysNoTechs.size()
                  << " 个, 未加入 effect, 详见 tech_always_no_potential.txt\n";
    }
    if (!infiniteTechs.empty()) {
        std::cout << "levels < 0 (无限重复) 的科技 " << infiniteTechs.size()
                  << " 个, 未加入 effect: ";
        std::vector<std::string> names;
        for (size_t i : infiniteTechs) names.push_back(techs[i].name);
        std::cout << join(names, ", ") << "\n";
    }
    if (!unreachableList.empty()) {
        std::cout << "因前置不可达而走不到, 未加入 effect 的科技 " << unreachableList.size()
                  << " 个:\n";
        for (const std::string& s : unreachableList) std::cout << "    " << s << "\n";
    }
    if (!addedBack.empty()) {
        std::cout << "补回的 is_ai = yes 科技 (" << addedBack.size()
                  << " 个, 均被名单内科技依赖):\n";
        for (const auto& p : addedBack) {
            std::cout << "    " << techs[p.first].name << " <- " << techs[p.second].name << "\n";
        }
    }
    if (!topoWithFlags.cycleMembers.empty()) {
        std::cout << "[警告] 加入 flag 推断依赖后存在循环依赖, 已尽力断开: "
                  << join(topoWithFlags.cycleMembers, ", ") << "\n";
    }
    if (opt.includeFlagDeps) {
        size_t n = 0;
        for (const Tech& t : techs) n += t.flagDeps.size();
        std::cout << "[提示] --include-flag-deps 解析出 " << n
                  << " 条由 has_country_flag 推断的依赖 (见 tech_dependencies.tsv)\n";
    }
    return 0;
}
