import kotlin.io.path.Path
import kotlin.io.path.appendText
import kotlin.io.path.exists
import kotlin.io.path.readLines
import kotlin.io.path.writeText
import kotlin.math.roundToInt

// ============================================================================
// component_vars.kts —— 生成组件属性 / 资源成本 / 缩放系数三份定义文件
//
// 由 ../../kotlinc -script ./component_vars.kts 运行（工作目录 = 本文件所在目录）
// 生成物：
//   components_attributes.log  属性定义（来自 components_attributes.csv）
//   components_resources.log   成本/维护定义（来自 components_resources.csv）
//   components_scales.log      @touhou_generic_scale_*_factor（**以 mod 现有值为准**）
// 再由 utils/component_vars_sync（cpp 步骤）合并进
//   common/scripted_variables/spth_component_variables.txt
// ============================================================================

data class ComponentResource(
    var clazz: String,
    var size: String,
    var type: String,
    var costAlloys: Float,
    var costSp: Float,
    var costFk: Float,
    var upkeepEnergy: Float,
    var upkeepSp: Float,
    var upkeepFk: Float,
) {
    override fun toString(): String {
        // spth_[common/crisis/special/[name]]_[weapon/utility]_[type]_[cost/upkeep]_[resource]
        return "@spth_${clazz}_${size}_${type}_cost_alloys = $costAlloys\n" +
                "@spth_${clazz}_${size}_${type}_cost_sr_lingli = $costSp\n" +
                "@spth_${clazz}_${size}_${type}_cost_sr_fuka = $costFk\n" +
                "@spth_${clazz}_${size}_${type}_upkeep_energy = $upkeepEnergy\n" +
                "@spth_${clazz}_${size}_${type}_upkeep_sr_lingli = $upkeepSp\n" +
                "@spth_${clazz}_${size}_${type}_upkeep_sr_fuka = $upkeepFk"
    }
}

data class ComponentAttribute(
    var name: String,
    var tag: String,
    var type: String,
    var damage_min: Float,
    var damage_max: Float,
    var windup_min: Float,
    var windup_max: Float,
    var fire_time: Float,
    var min_range: Float,
    var range: Float,
    var accuracy: Float,
    var tracking: Float,
    var shield_penetration: Float,
    var armor_penetration: Float,
    var shield_damage: Float,
    var armor_damage: Float,
    var hull_damage: Float,
    var power: Int,
) {
    override fun toString(): String {
        return "@spth_${name}_${tag}_${type}_damage_min = $damage_min\n" +
                "@spth_${name}_${tag}_${type}_damage_max = $damage_max\n" +
                "@spth_${name}_${tag}_${type}_windup_min = $windup_min\n" +
                "@spth_${name}_${tag}_${type}_windup_max = $windup_max\n" +
                "@spth_${name}_${tag}_${type}_fire_time = $fire_time\n" +
                "@spth_${name}_${tag}_${type}_min_range = $min_range\n" +
                "@spth_${name}_${tag}_${type}_range = $range\n" +
                "@spth_${name}_${tag}_${type}_accuracy = $accuracy\n" +
                "@spth_${name}_${tag}_${type}_tracking = $tracking\n" +
                "@spth_${name}_${tag}_${type}_shield_penetration = $shield_penetration\n" +
                "@spth_${name}_${tag}_${type}_armor_penetration = $armor_penetration\n" +
                "@spth_${name}_${tag}_${type}_shield_damage = $shield_damage\n" +
                "@spth_${name}_${tag}_${type}_armor_damage = $armor_damage\n" +
                "@spth_${name}_${tag}_${type}_hull_damage = $hull_damage\n" +
                "@spth_${name}_${tag}_${type}_power = $power"
    }
    fun toCSVLine(): String {
        return "$name,$tag,$type,${damage_min.roundToInt()},${damage_max.roundToInt()},${windup_min.roundToInt()},${windup_max.roundToInt()},${fire_time.roundToInt()},${min_range.roundToInt()},${range.roundToInt()},$accuracy,$tracking,$shield_penetration,$armor_penetration,$shield_damage,$armor_damage,$hull_damage,$power"
    }
}

fun readComponentResourceCSV(content: List<String>): List<ComponentResource> {
    val res = ArrayDeque<ComponentResource>()
    content.forEach { line ->
        if (line.isNotEmpty() && !line.startsWith("#")) {
            val data = line.split(',')
            res.addLast(
                ComponentResource(
                    data[0], data[1], data[2],
                    data[3].toFloat(), data[4].toFloat(), data[5].toFloat(),
                    data[6].toFloat(), data[7].toFloat(), data[8].toFloat(),
                )
            )
        }
    }
    return res
}

fun readComponentAttributeCSV(content: List<String>): List<ComponentAttribute> {
    val res = ArrayDeque<ComponentAttribute>()
    content.forEach { line ->
        if (line.isNotEmpty() && !line.startsWith("#")) {
            val data = line.split(',')
            res.addLast(
                ComponentAttribute(
                    data[0], data[1], data[2],
                    data[3].toFloat(), data[4].toFloat(), data[5].toFloat(), data[6].toFloat(),
                    data[7].toFloat(), data[8].toFloat(), data[9].toFloat(), data[10].toFloat(),
                    data[11].toFloat(), data[12].toFloat(), data[13].toFloat(), data[14].toFloat(),
                    data[15].toFloat(), data[16].toFloat(), data[17].toInt(),
                )
            )
        }
    }
    return res
}

/** 从 mod 现有 scripted_variables 里读出 @name = value（用于"以 mod 为准"） */
fun parseExistingDefs(pathStr: String): Map<String, String> {
    val out = HashMap<String, String>()
    val path = Path(pathStr)
    if (!path.exists()) return out
    path.readLines().forEach { raw ->
        val line = raw.substringBefore('#').trim()
        if (line.startsWith("@")) {
            val i = line.indexOf('=')
            if (i > 0) out[line.substring(1, i).trim()] = line.substring(i + 1).trim()
        }
    }
    return out
}

// 以脚本所在目录为基准（用 ../../kotlinc -script ./component_vars.kts 时即 utils/）
val scriptDir = Path("./")

val ATTR_CSV = scriptDir.resolve("components_attributes.csv")
val RES_CSV = scriptDir.resolve("components_resources.csv")
val TARGET_VARS = scriptDir.resolve("../common/scripted_variables/spth_component_variables.txt")
val OUT_ATTR = scriptDir.resolve("components_attributes.log")
val OUT_RES = scriptDir.resolve("components_resources.log")
val OUT_SCALE = scriptDir.resolve("components_scales.log")

val typesCached = sortedSetOf<String>()   // 排序输出，保证多次运行结果一致

// ---------- 1) 属性 ----------
OUT_ATTR.writeText("")
readComponentAttributeCSV(ATTR_CSV.readLines().drop(1)).forEach {
    OUT_ATTR.appendText(it.toString() + '\n')
    typesCached.add(it.type)
}

// ---------- 2) 资源成本 / 维护（以前这段是注释掉的，现在纳入流程） ----------
OUT_RES.writeText("")
readComponentResourceCSV(RES_CSV.readLines().drop(1)).forEach {
    OUT_RES.appendText(it.toString() + '\n')
}

// ---------- 3) 缩放系数：以 mod 现有值为准，缺失的 type 才用 1.0 ----------
val existing = parseExistingDefs(TARGET_VARS.toString())
OUT_SCALE.writeText("")
typesCached.forEach { t ->
    listOf("damage", "windup", "fire_time").forEach { kind ->
        val key = "touhou_generic_scale_${kind}_${t}_factor"
        val value = existing[key] ?: "1.0"
        OUT_SCALE.appendText("@$key = $value\n")
    }
}

println("generated: ${OUT_ATTR.toAbsolutePath().normalize()} (${typesCached.size} types)")
println("generated: ${OUT_RES.toAbsolutePath().normalize()}")
println("generated: ${OUT_SCALE.toAbsolutePath().normalize()} (values taken from ${TARGET_VARS.toAbsolutePath().normalize()})")
