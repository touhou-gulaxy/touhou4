fun genShipSizeSprites(spriteSheet: String, index: Int) {
    println("""
        spriteType = {
            name = "GFX_${spriteSheet}_$index"
            sprite_sheet_sprite_type = "GFX_$spriteSheet"
            default_frame = $index
        }
        spriteType = {
            name = "GFX_${spriteSheet}_top_$index"
            sprite_sheet_sprite_type = "GFX_${spriteSheet}_top"
            default_frame = $index
        }
        spriteType = {
            name = "GFX_${spriteSheet}_top_damaged_$index"
            sprite_sheet_sprite_type = "GFX_${spriteSheet}_top_damaged"
            default_frame = $index
        }
    """.trimIndent())
}
(1..7).forEach {
    genShipSizeSprites("spth_boss_ship_sizes", it)
}
(11..14).forEach {
    genShipSizeSprites("spth_boss_ship_sizes", it)
}
