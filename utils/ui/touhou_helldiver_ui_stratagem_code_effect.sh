#!/bin/bash

for idx in {1..8}; do
    for dir in up down left right; do
        cat <<EOF
touhou_helldiver_ui_stratagem_code_${idx}_${dir} = {
    potential = {
        from = {
            touhou_helldiver_stratagem_code_check_confirmed = { idx = ${idx} rel = "<" }
            touhou_helldiver_stratagem_code_check_direction = { idx = ${idx} dir = ${dir} }
        }
    }
    allow = { }
    effect = { }
}
touhou_helldiver_ui_stratagem_code_${idx}_${dir}_alt = {
    potential = {
        from = {
            touhou_helldiver_stratagem_code_check_confirmed = { idx = ${idx} rel = ">=" }
            touhou_helldiver_stratagem_code_check_direction = { idx = ${idx} dir = ${dir} }
        }
    }
    allow = { }
    effect = { }
}
EOF
    done
done
