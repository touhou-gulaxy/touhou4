#!/bin/bash

for idx in {1..8}; do
    n=$((idx - 1))
    for dir in up down left right; do
        cat <<EOF
        # idx = ${idx}, ${dir}, false
        effectButtonType = {
            name = "touhou_helldiver_ui_stratagem_code_${idx}_${dir}"
            position = { x = @[touhou_helldiver_ui_stratagem_code_1_x + ${n} * touhou_helldiver_ui_stratagem_code_dx] y = @touhou_helldiver_ui_stratagem_code_1_y }
            orientation = upper_left
            spriteType = "GFX_touhou_ext_helldiver_arrow_${dir}"
            buttonFont = "spth_cg_16b"
            buttonText = ""
            borderSize = { x = 100 y = 100 }
            oversound = "mouse_over"
            clicksound = "ui_click_research_tab"
            effect = touhou_helldiver_ui_stratagem_code_${idx}_${dir}
            alwaysTransparent = yes
        }
        # idx = ${idx}, ${dir}, true
        effectButtonType = {
            name = "touhou_helldiver_ui_stratagem_code_${idx}_${dir}_alt"
            position = { x = @[touhou_helldiver_ui_stratagem_code_1_x + ${n} * touhou_helldiver_ui_stratagem_code_dx] y = @touhou_helldiver_ui_stratagem_code_1_y }
            orientation = upper_left
            spriteType = "GFX_touhou_ext_helldiver_arrow_${dir}_alt"
            buttonFont = "spth_cg_16b"
            buttonText = ""
            borderSize = { x = 100 y = 100 }
            oversound = "mouse_over"
            clicksound = "ui_click_research_tab"
            effect = touhou_helldiver_ui_stratagem_code_${idx}_${dir}_alt
            alwaysTransparent = yes
        }
EOF
    done
done