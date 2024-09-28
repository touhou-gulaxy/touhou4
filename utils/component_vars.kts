import kotlin.io.path.Path
import kotlin.io.path.readLines

data class ComponentResource(
    val clazz: String,
    val size: String,
    val type: String,
    val costAlloys: Float,
    val costSp: Float,
    val costFk: Float,
    val upkeepEnergy: Float,
    val upkeepSp: Float,
    val upkeepFk: Float,
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
    val name: String,
    val tag: String,
    val type: String,
    val damage_min: Float,
    val damage_max: Float,
    val windup_min: Float,
    val windup_max: Float,
    val fire_time: Float,
    val min_range: Float,
    val range: Float,
    val accuracy: Float,
    val tracking: Float,
    val shield_penetration: Float,
    val armor_penetration: Float,
    val shield_damage: Float,
    val armor_damage: Float,
    val hull_damage: Float,
    val power: Int,
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
}

fun readComponentResourceCSV(content: List<String>): List<ComponentResource> {
    val res = ArrayDeque<ComponentResource>()
    content.forEach { line ->
        if (line.isNotEmpty()) {
            // println(line)
            val data = line.split(',')
            res.addLast(
                ComponentResource(
                    data[0],
                    data[1],
                    data[2],
                    data[3].toFloat(),
                    data[4].toFloat(),
                    data[5].toFloat(),
                    data[6].toFloat(),
                    data[7].toFloat(),
                    data[8].toFloat(),
                )
            )
        }
    }
    return res
}

fun readComponentAttributeCSV(content: List<String>): List<ComponentAttribute> {
    val res = ArrayDeque<ComponentAttribute>()
    content.forEach { line ->
        if (line.isNotEmpty()) {
            // println(line)
            val data = line.split(',')
            res.addLast(
                ComponentAttribute(
                    data[0],
                    data[1],
                    data[2],
                    data[3].toFloat(),
                    data[4].toFloat(),
                    data[5].toFloat(),
                    data[6].toFloat(),
                    data[7].toFloat(),
                    data[8].toFloat(),
                    data[9].toFloat(),
                    data[10].toFloat(),
                    data[11].toFloat(),
                    data[12].toFloat(),
                    data[13].toFloat(),
                    data[14].toFloat(),
                    data[15].toFloat(),
                    data[16].toFloat(),
                    data[17].toInt(),
                )
            )
        }
    }
    return res
}

readComponentResourceCSV(Path("./components_resources.csv").readLines().drop(1)).forEach {
    println(it)
}
//readComponentAttributeCSV(Path("./components_attributes.csv").readLines().drop(1)).forEach {
//    println(it)
//}
