fun generateSiteEvent(prefix: String, index: Int) {
    println(
        """
        planet_event = {
        	id = ${prefix}.$index
        	title = "${prefix}.$index"
        	desc = "${prefix}.${index}.desc"
        	picture = "GFX_evt_burning_city"
        	is_triggered_only = yes
        	
        	option = {
        		name = "${prefix}.${index}.a"
        		small_artifact_reward = yes
        	}
        }
    """.trimIndent()
    )
}
fun generateSiteEventLocals(prefix: String, index: Int) {
    println(
        """
        ${prefix}.$index: "REPLACE_ME"
        ${prefix}.$index.desc: "REPLACE_ME"
        """.trimIndent()
    )
}

(33..41).forEach {
    generateSiteEvent("th_arc", it)
}
(33..41).forEach {
    generateSiteEventLocals("th_arc", it)
}
