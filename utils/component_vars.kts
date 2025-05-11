import kotlin.io.path.Path
import kotlin.io.path.readLines
import kotlin.math.roundToInt

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

//readComponentResourceCSV(Path("./components_resources.csv").readLines().drop(1)).forEach {
//    println(it)
//}
readComponentAttributeCSV(Path("./components_attributes.csv").readLines().drop(1)).forEach {
    println(it)
}
//readComponentAttributeCSV(Path("./components_attributes.csv").readLines().drop(1)).map {
//    it.name += "_mutation"
//    it.power = 0
//    it.range /= 1.25f
//    it.min_range /= 1.25f
//    it.damage_min /= 2.025f
//    it.damage_max /= 2.025f
//    it.fire_time /= 1.10f
//    it
//}.forEach {
//    println(it.toCSVLine())
//}

