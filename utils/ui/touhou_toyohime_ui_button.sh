#!/bin/bash

for slot in {0..4}; do
	echo "		##################"
	echo "		# char at slot ${slot}"
	echo "		##################"
	for pos in {0..3}; do
		for color in white red; do
			echo "		# char at pos ${pos} with ${color}"
			for char in {1..8}; do
				cat <<EOF
		effectButtonType = {
			name = "touhou_toyohime_puzzle_slot_${slot}_pos_${pos}_${color}_char${char}"
			position = { x = @[touhou_toyohime_puzzle_slot_0_x + $slot * touhou_toyohime_puzzle_slot_dx] y = @[touhou_toyohime_puzzle_pos_0_x + $pos * touhou_toyohime_puzzle_pos_dx] }
			orientation = upper_left
			spriteType = "GFX_btn_touhou_automate_puzzle_${color}_char${char}"
			buttonFont = "spth_cg_16b"
			buttonText = ""
			borderSize = { x = 100 y = 100 }
			oversound = "mouse_over"
			clicksound = "ui_click_research_tab"
			effect = touhou_toyohime_puzzle_slot_${slot}_pos_${pos}_${color}_char${char}
			alwaysTransparent = yes
		}
EOF
			done
		done
	done
done

for slot in {0..4}; do
	echo "		##################"
	echo "		# char at slot ${slot}"
	echo "		##################"
	for pos in {0..3}; do
		echo "		# char at pos ${pos}"
		cat <<EOF
		effectButtonType = {
			name = "touhou_toyohime_puzzle_check_${slot}_${pos}_null"
			position = { x = @[touhou_toyohume_puzzle_right_x + $slot * touhou_toyohume_puzzle_right_dxy] y = @[touhou_toyohume_puzzle_right_y + $pos * touhou_toyohume_puzzle_right_dxy] }
			orientation = upper_left
			spriteType = "GFX_btn_touhou_automate_puzzle_example_null"
			buttonFont = "spth_cg_16b"
			buttonText = ""
			borderSize = { x = 100 y = 100 }
			oversound = "mouse_over"
			clicksound = "ui_click_research_tab"
			effect = touhou_toyohime_puzzle_check_${slot}_${pos}_null
			alwaysTransparent = yes
		}
EOF
		for char in {1..8}; do
			cat <<EOF
		effectButtonType = {
			name = "touhou_toyohime_puzzle_check_${slot}_${pos}_red_char${char}"
			position = { x = @[touhou_toyohume_puzzle_right_x + $slot * touhou_toyohume_puzzle_right_dxy] y = @[touhou_toyohume_puzzle_right_y + $pos * touhou_toyohume_puzzle_right_dxy] }
			orientation = upper_left
			spriteType = "GFX_btn_touhou_automate_puzzle_red_char${char}"
			buttonFont = "spth_cg_16b"
			buttonText = ""
			borderSize = { x = 100 y = 100 }
			oversound = "mouse_over"
			clicksound = "ui_click_research_tab"
			effect = touhou_toyohime_puzzle_check_${slot}_${pos}_red_char${char}
			alwaysTransparent = yes
		}
EOF
		done
	done
done
