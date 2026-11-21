#!/bin/bash

cat <<EOF
touhou_toyohime_puzzle_move_up = {
	potential = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_success } } }
	allow = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_lock } } }
	effect = {
		from = {
			set_country_flag = touhou_toyohime_puzzle_input_lock
			if = {
				limit = { NOT = { is_variable_set = touhou_toyohime_puzzle_sel_pos } }
				set_variable = { which = touhou_toyohime_puzzle_sel_pos value = 0 }
			}
			touhou_toyohime_puzzle_move_char_array = { up = yes }
			# check stratagem code seq
			# if = {
			# 	limit = {
			# 		check_variable_arithmetic = {
			# 			which = value:touhou_toyohime_puzzle_get_char
			# 			value = value:touhou_toyohime_puzzle_check_char
			# 		}
			# 	}
			# 	fire_on_action = {
			# 		on_action = on_touhou_toyohime_puzzle_check_success
			# 	}
			# 	set_country_flag = touhou_toyohime_puzzle_input_success
			# }
			remove_country_flag = touhou_toyohime_puzzle_input_lock
		}
	}
}
touhou_toyohime_puzzle_move_down = {
	potential = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_success } } }
	allow = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_lock } } }
	effect = {
		from = {
			set_country_flag = touhou_toyohime_puzzle_input_lock
			if = {
				limit = { NOT = { is_variable_set = touhou_toyohime_puzzle_sel_pos } }
				set_variable = { which = touhou_toyohime_puzzle_sel_pos value = 0 }
			}
			touhou_toyohime_puzzle_move_char_array = { down = yes }
			# check stratagem code seq
			# if = {
			# 	limit = {
			# 		check_variable_arithmetic = {
			# 			which = value:touhou_toyohime_puzzle_get_char
			# 			value = value:touhou_toyohime_puzzle_check_char
			# 		}
			# 	}
			# 	fire_on_action = {
			# 		on_action = on_touhou_toyohime_puzzle_check_success
			# 	}
			# 	set_country_flag = touhou_toyohime_puzzle_input_success
			# }
			remove_country_flag = touhou_toyohime_puzzle_input_lock
		}
	}
}
touhou_toyohime_puzzle_move_left = {
	potential = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_success } } }
	allow = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_lock } } }
	effect = {
		from = {
			set_country_flag = touhou_toyohime_puzzle_input_lock
			if = {
				limit = { NOT = { is_variable_set = touhou_toyohime_puzzle_sel_slot } }
				set_variable = { which = touhou_toyohime_puzzle_sel_slot value = 0 }
			}
			change_variable = { which = touhou_toyohime_puzzle_sel_slot value = -1 }
			if = {
				limit = { check_variable = { which = touhou_toyohime_puzzle_sel_slot value < 0 } }
				set_variable = { which = touhou_toyohime_puzzle_sel_slot value = 0 }
			}
			# check stratagem code seq
			# if = {
			# 	limit = {
			# 		check_variable_arithmetic = {
			# 			which = value:touhou_toyohime_puzzle_get_char
			# 			value = value:touhou_toyohime_puzzle_check_char
			# 		}
			# 	}
			# 	fire_on_action = {
			# 		on_action = on_touhou_toyohime_puzzle_check_success
			# 	}
			# 	set_country_flag = touhou_toyohime_puzzle_input_success
			# }
			remove_country_flag = touhou_toyohime_puzzle_input_lock
		}
	}
}
touhou_toyohime_puzzle_move_right = {
	potential = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_success } } }
	allow = { from = { NOT = { has_country_flag = touhou_toyohime_puzzle_input_lock } } }
	effect = {
		from = {
			set_country_flag = touhou_toyohime_puzzle_input_lock
			if = {
				limit = { NOT = { is_variable_set = touhou_toyohime_puzzle_sel_slot } }
				set_variable = { which = touhou_toyohime_puzzle_sel_slot value = 0 }
			}
			change_variable = { which = touhou_toyohime_puzzle_sel_slot value = 1 }
			if = {
				limit = { check_variable = { which = touhou_toyohime_puzzle_sel_slot value > 4 } }
				set_variable = { which = touhou_toyohime_puzzle_sel_slot value = 4 }
			}
			# check stratagem code seq
			# if = {
			# 	limit = {
			# 		check_variable_arithmetic = {
			# 			which = value:touhou_toyohime_puzzle_get_char
			# 			value = value:touhou_toyohime_puzzle_check_char
			# 		}
			# 	}
			# 	fire_on_action = {
			# 		on_action = on_touhou_toyohime_puzzle_check_success
			# 	}
			# 	set_country_flag = touhou_toyohime_puzzle_input_success
			# }
			remove_country_flag = touhou_toyohime_puzzle_input_lock
		}
	}
}


EOF

for slot in {0..4}; do
	echo "##################"
	echo "# char at slot ${slot}"
	echo "##################"
	for pos in {0..3}; do
		for color in white red; do
			if [ $color == "red" ]; then
				check_pos_char="# "
			else
				check_pos_char=""
			fi
			echo "# char at pos ${pos} with ${color}"
			for char in {1..8}; do
				cat <<EOF
touhou_toyohime_puzzle_slot_${slot}_pos_${pos}_${color}_char${char} = {
	potential = {
		from = {
			touhou_toyohime_puzzle_check_slot_is_${color} = { slot = ${slot} }
			touhou_toyohime_puzzle_check_pos_char = { slot = ${slot} pos = ${pos} char = ${char} }
		}
	}
	allow = { }
	effect = { }
}
EOF
			done
		done
	done
done

for slot in {0..4}; do
	echo "##################"
	echo "# check at slot ${slot}"
	echo "##################"
	for pos in {0..3}; do
		echo "# char at pos ${pos}"
		cat <<EOF
touhou_toyohime_puzzle_check_${slot}_${pos}_null = {
	potential = {
		from = {
			NOT = { touhou_toyohime_puzzle_check_is_check_item = { slot = ${slot} pos = ${pos} } }
		}
	}
	allow = { }
	effect = { }
}
EOF
		for char in {1..8}; do
				cat <<EOF
touhou_toyohime_puzzle_check_${slot}_${pos}_red_char${char} = {
	potential = {
		from = {
			touhou_toyohime_puzzle_check_is_check_item = { slot = ${slot} pos = ${pos} }
			touhou_toyohime_puzzle_check_char_at_check = { slot = ${slot} char = ${char} }
		}
	}
	allow = { }
	effect = { }
}
EOF
		done
	done
done
