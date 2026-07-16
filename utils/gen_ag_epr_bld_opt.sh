#!/bin/bash

generate()
{
  local ag_ship_key="$1"
  echo ""
  echo " ag_exploration_story.10.shipyard.${ag_ship_key}.build:0 \"建造\$ag_exploration.1.fleet.${ag_ship_key}$\""
  echo " ag_exploration_story.10.shipyard.${ag_ship_key}.cancel:0 \"取消建造\$ag_exploration.1.fleet.${ag_ship_key}$\""
  echo " ag_exploration_story.10.shipyard.${ag_ship_key}.build_count:0 \"预计建造时间：§Y\$@ag_exploration_ship_${ag_ship_key}_buildtime$§!£time£\n\$ag_exploration.1.fleet.${ag_ship_key}\$建造数量：\n -正在建造：§Y[ag_exploraion_fleet_shipyard_build_${ag_ship_key}_ongoing]§!\n -计划建造：§Y[ag_exploraion_fleet_shipyard_build_${ag_ship_key}_queue]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.energy:0 \"£energy£§H\$energy$§R不足：\$@ag_exploration_${ag_ship_key}_cost_energy$/[Root.ag_exploration_resource_energy]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.minerals:0 \"£minerals£§H\$minerals$§R不足：\$@ag_exploration_${ag_ship_key}_cost_minerals$/[Root.ag_exploration_resource_minerals]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.food:0 \"£food£§H\$food$§R不足：\$@ag_exploration_${ag_ship_key}_cost_food$/[Root.ag_exploration_resource_food]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.alloys:0 \"£alloys£§H\$alloys$§R不足：\$@ag_exploration_${ag_ship_key}_cost_alloys$/[Root.ag_exploration_resource_alloys]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.consumer_goods:0 \"£consumer_goods£§H\$consumer_goods$§R不足：\$@ag_exploration_${ag_ship_key}_cost_consumer_goods$/[Root.ag_exploration_resource_consumer_goods]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.volatile_motes:0 \"£volatile_motes£§H\$volatile_motes$§R不足：\$@ag_exploration_${ag_ship_key}_cost_volatile_motes$/[Root.ag_exploration_resource_volatile_motes]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.exotic_gases:0 \"£exotic_gases£§H\$exotic_gases$§R不足：\$@ag_exploration_${ag_ship_key}_cost_exotic_gases$/[Root.ag_exploration_resource_exotic_gases]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.rare_crystals:0 \"£rare_crystals£§H\$rare_crystals$§R不足：\$@ag_exploration_${ag_ship_key}_cost_rare_crystals$/[Root.ag_exploration_resource_rare_crystals]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.sr_living_metal:0 \"£sr_living_metal£§H\$sr_living_metal$§R不足：\$@ag_exploration_${ag_ship_key}_cost_sr_living_metal$/[Root.ag_exploration_resource_sr_living_metal]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.nanites:0 \"£nanites£§H\$nanites$§R不足：\$@ag_exploration_${ag_ship_key}_cost_nanites$/[Root.ag_exploration_resource_nanites]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.sr_dark_matter:0 \"£sr_dark_matter£§H\$sr_dark_matter$§R不足：\$@ag_exploration_${ag_ship_key}_cost_sr_dark_matter$/[Root.ag_exploration_resource_sr_dark_matter]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.sr_zro:0 \"£sr_zro£§H\$sr_zro$§R不足：\$@ag_exploration_${ag_ship_key}_cost_sr_zro$/[Root.ag_exploration_resource_sr_zro]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.minor_artifacts:0 \"£minor_artifacts£§H\$minor_artifacts$§R不足：\$@ag_exploration_${ag_ship_key}_cost_minor_artifacts$/[Root.ag_exploration_resource_minor_artifacts]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.astral_threads:0 \"£astral_threads£§H\$astral_threads$§R不足：\$@ag_exploration_${ag_ship_key}_cost_astral_threads$/[Root.ag_exploration_resource_astral_threads]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.biomass:0 \"£biomass£§H\$biomass$§R不足：\$@ag_exploration_${ag_ship_key}_cost_biomass$/[Root.ag_exploration_resource_biomass]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.entropy_crystals:0 \"£entropy_crystals£§H\$entropy_crystals$§R不足：\$@ag_exploration_${ag_ship_key}_cost_entropy_crystals$/[Root.ag_exploration_resource_entropy_crystals]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.sr_lingli:0 \"£sr_lingli£§H\$sr_lingli$§R不足：\$@ag_exploration_${ag_ship_key}_cost_sr_lingli$/[Root.ag_exploration_resource_sr_lingli]§!\""
  echo " ag_exploration.1.shipyard_no_resources.${ag_ship_key}.sr_fuka:0 \"£sr_fuka£§H\$sr_fuka$§R不足：\$@ag_exploration_${ag_ship_key}_cost_sr_fuka$/[Root.ag_exploration_resource_sr_fuka]§!\""
}

generate th_battle_cruiser
generate spth_kamikakushi_cruiser
generate touhou_command_dreadnought
generate lunar_capital_battleship
generate lunar_capital_escort
generate lunar_capital_frigate
