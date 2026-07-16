#!/bin/bash

for faction in lunar_capital kamikakushi; do
    echo "# ${faction}"
    for ship_size in small medium large extra_large titan; do
        echo "
# @touhou_${faction}_corrupted_${ship_size}_sublight_speed_basic = 300
# @touhou_${faction}_corrupted_${ship_size}_hull_hp = 6400
# @touhou_${faction}_corrupted_${ship_size}_armor_hp = 0
# @touhou_${faction}_corrupted_${ship_size}_shield_hp = 9600
# @touhou_${faction}_corrupted_${ship_size}_evasion_add = 30
# @touhou_${faction}_corrupted_${ship_size}_accuracy_add = 10
# @touhou_${faction}_corrupted_${ship_size}_tracking_add = 10
# @touhou_${faction}_corrupted_${ship_size}_shield_damage = 0.25
# @touhou_${faction}_corrupted_${ship_size}_armor_damage = 0.25
# @touhou_${faction}_corrupted_${ship_size}_hull_damage = 0.25
# @touhou_${faction}_corrupted_${ship_size}_shield_efficiency = 5.0
# @touhou_${faction}_corrupted_${ship_size}_armor_efficiency = 0
# @touhou_${faction}_corrupted_${ship_size}_shield_hardening_add = 10.0
# @touhou_${faction}_corrupted_${ship_size}_armor_hardening_add = 0
# @touhou_${faction}_corrupted_${ship_size}_target_weight_mult = -0.50
# @touhou_${faction}_corrupted_${ship_size}_armor_regen_add_perc = 0
# @touhou_${faction}_corrupted_${ship_size}_shield_regen_add_perc = 1.0
# @touhou_${faction}_corrupted_${ship_size}_hull_regen_add_perc = 0.25
# @touhou_${faction}_corrupted_${ship_size}_armor_regen_add_factor = 0.025
# @touhou_${faction}_corrupted_${ship_size}_shield_regen_add_factor = 0.025
# @touhou_${faction}_corrupted_${ship_size}_hull_regen_add_factor = 0.025
# @touhou_${faction}_corrupted_${ship_size}_evasion_mult = 0
# @touhou_${faction}_corrupted_${ship_size}_accuracy_mult = 0
# @touhou_${faction}_corrupted_${ship_size}_tracking_mult = 0
# @touhou_${faction}_corrupted_${ship_size}_size_multiplier = 5
# @touhou_${faction}_corrupted_${ship_size}_combat_size_multiplier = 5
# @touhou_${faction}_corrupted_${ship_size}_fleet_slot_size = 2"
    done
done
