#### 现有组件

- #### 武器

总体为动能专精反护盾，能量专精反装甲。红魔馆武器数值更大，幻想乡种类更多。

可能需要修正的问题：

1. 梦想封印梦想天生射速过快
2. 楼观剑只有S槽
3. 草苗御币需要重新平衡
4. 贤者之石缺少S槽及S槽本地化
5. 收缩的世界只有L槽，另外两个定义了没有本地化

| 名称    | 槽位<br/>射角  | tag                | 伤害          | 冷却<br/>发射时间  | 平均伤害   | 射程     | 命中<br/>追踪   | 修正                                   | ai tag      |
|-------|------------|--------------------|-------------|--------------|--------|--------|-------------|--------------------------------------|-------------| 
| 冈格尼尔  | X<br/>60.0 | 动能<br/>冈格尼尔<br/>灵力 | 4951~4951   | 1~19<br/>200 | 235.76 | 10~300 | 1.0<br/>1.0 | +100%护盾穿透<br/>+100%装甲伤害              | 炮击          |
| 冈格尼尔  | T<br/>25.0 | 动能<br/>冈格尼尔<br/>灵力 | 10514~10514 | 1~19<br/>200 | 500.67 | 30~300 | 1.0<br/>1.0 | +200%护盾穿透<br/>+200%装甲伤害<br/>+50%船体伤害 | 炮击          |
| 魔炮    | X<br/>60.0 | 能量<br/>灵力          | 4951~4951   | 1~19<br/>200 | 235.76 | 10~300 | 1.0<br/>1.0 | +100%装甲穿透<br/>+100%护盾伤害              | 炮击          |
| 魔炮    | T<br/>25.0 | 能量<br/>灵力          | 10514~10514 | 1~19<br/>200 | 500.67 | 30~300 | 1.0<br/>1.0 | +200%装甲穿透<br/>+200%护盾伤害<br/>+50%船体伤害 | 炮击          |
| 魔炮    | S<br/>全向   | 能量<br/>灵力          | 120~120     | 3~7<br/>20   | 48.00  | 100    | 1.0<br/>0.6 | +100%装甲穿透<br/>+50%护盾伤害               | 反护盾         |
| 魔炮    | M<br/>全向   | 能量<br/>灵力          | 256～256     | 3～7<br/>30   | 73.14  | 150    | 1.0<br/>0.5 | +100%装甲穿透<br/>+75%护盾伤害               | 反护盾         |
| 魔炮    | L<br/>全向   | 能量<br/>灵力          | 512~512     | 5~15<br/>30  | 128.00 | 10~200 | 1.0<br/>0.6 | +100%装甲穿透<br/>+100%护盾伤害              | 反护盾         |
| 梦想封印  | S<br/>全向   | 能量<br/>灵力          | 12～12       | 1～2<br/>1    | 48.00  | 100    | 1.0<br/>0.3 | +100%装甲穿透<br/>+50%护盾伤害               | 反护盾         |
| 梦想封印  | M<br/>全向   | 能量<br/>灵力          | 18~18       | 1~2<br/>1    | 72.00  | 150    | 1.0<br/>0.3 | +100%装甲穿透<br/>+75%护盾伤害               | 反护盾         |
| 梦想封印  | L<br/>全向   | 能量<br/>灵力          | 31~31       | 1~2<br/>1    | 124.00 | 200    | 1.0<br/>0.3 | +100%装甲穿透<br/>+100%护盾伤害              | 反护盾         |
| 梦想天生  | M<br/>全向   | 能量<br/>灵力          | 14~14       | 1~2<br/>0.5  | 28.00  | 200    | 1.0<br/>0.8 | +50%护盾穿透<br/>+50%装甲穿透                | 反护盾<br/>反装甲 |
| 梦想天生  | M<br/>全向   | 能量<br/>灵力          | 22~23       | 1~2<br/>0.5  | 45.00  | 200    | 1.0<br/>0.8 | +50%护盾穿透<br/>+50%装甲穿透                | 反护盾<br/>反装甲 |
| 泄矢铁轮  | S<br/>全向   | 动能<br/>灵力          | 120~120     | 3~7<br/>20   | 48.00  | 100    | 1.0<br/>0.6 | +100%装甲穿透<br/>+50%护盾伤害               | 反装甲         |
| 泄矢铁轮  | M<br/>全向   | 动能<br/>灵力          | 256~256     | 3~7<br/>30   | 73.14  | 150    | 1.0<br/>0.6 | +100%装甲穿透<br/>+75%护盾伤害               | 反装甲         |
| 泄矢铁轮  | L<br/>全向   | 动能<br/>灵力          | 512~512     | 5~15<br/>30  | 128.00 | 200    | 1.0<br/>0.6 | +100%装甲穿透<br/>+100%护盾伤害              | 反装甲         |
| 草苗御币  | S<br/>全向   | 动能<br/>能量<br/>灵力   | 50~75       | 10~20<br/>10 | 25.00  | 100    | 0.8<br/>0.7 | +100%护盾穿透<br/>+100%装甲穿透              | 反护盾<br/>反装甲 |
| 草苗御币  | M<br/>全向   | 动能<br/>能量<br/>灵力   | 100~113     | 10~20<br/>10 | 42.60  | 200    | 0.8<br/>0.7 | +100%护盾穿透<br/>+100%装甲穿透              | 反护盾<br/>反装甲 |
| 草苗御币  | L<br/>全向   | 动能<br/>能量<br/>灵力   | 175~200     | 10~20<br/>10 | 75.00  | 300    | 0.8<br/>0.7 | +100%护盾穿透<br/>+100%装甲穿透              | 反护盾<br/>反装甲 |
| 楼观剑   | S<br/>全向   | 动能<br/>灵力          | 40~1000     | 2~20<br/>10  | 247.62 | 15     | 1.0<br/>0.9 | +100%护盾穿透<br/>+100%装甲穿透<br/>+50%船体伤害 | 反船体         |
| 贤者之石  | S<br/>全向   | 能量<br/>贤者之石<br/>灵力 | 195~195     | 10~30<br/>20 | 65.00  | 150    | 1.0<br/>0.5 | +100%装甲穿透<br/>+100%护盾伤害              | 反护盾         |
| 贤者之石  | M<br/>全向   | 能量<br/>贤者之石<br/>灵力 | 390~390     | 10~30<br/>20 | 130.00 | 200    | 1.0<br/>0.5 | +100%装甲穿透<br/>+150%护盾伤害              | 反护盾         |
| 贤者之石  | L<br/>全向   | 能量<br/>贤者之石<br/>灵力 | 514~514     | 10~30<br/>20 | 171.00 | 250    | 1.0<br/>0.5 | +100%装甲穿透<br/>+200%护盾伤害              | 反护盾         |
| 收缩的世界 | S<br/>全向   | 动能<br/>时间<br/>灵力   | 195~195     | 10~30<br/>20 | 65.00  | 150    | 1.0<br/>0.5 | +100%护盾穿透<br/>+100%装甲伤害              | 反装甲         |
| 收缩的世界 | M<br/>全向   | 动能<br/>时间<br/>灵力   | 390~390     | 10~30<br/>20 | 130.00 | 200    | 1.0<br/>0.5 | +100%护盾穿透<br/>+150%装甲伤害              | 反装甲         |
| 收缩的世界 | L<br/>全向   | 动能<br/>时间<br/>灵力   | 514~514     | 10~30<br/>20 | 171.00 | 250    | 1.0<br/>0.5 | +100%护盾穿透<br/>+200%装甲伤害              | 反装甲         |
|       |            |                    |             |              |        |        |             |                                      |             |
|       |            |                    |             |              |        |        |             |                                      |             |

- #### A槽

| 名称        | 图标                      | 修正                                                     | power | extra |
|-----------|-------------------------|--------------------------------------------------------|-------|-------|
| 灵力相位隐形发生器 | GFX_ship_part_cloak_psi | +16隐形强度<br/>隐形时-50%舰船速度                                | -150  | 数量上限2 |
| 式神解编码器    | 未定，模仿神秘解码器&神秘编码器        | +2撤离机会<br/>+25%闪避<br/>+10命中<br/>+10追踪                  | -100  |       |
| 灵力辅助反应室   | 未定，模仿afterburner        | +10闪避<br/>+40%舰船速度                                     | -125  |       |
| 核融合能量模块   | 未定，模仿反应堆增压器             |                                                        | +750  |       |
| 月都损管机械    | 未定，模仿纳米修复系统             | +25每日船体回复<br/>+50%每日船体恢复<br/>+50%每日护盾恢复<br/>+50%每日装甲恢复 | -150  |       |
| 舰体硬化单元    | 未定，模仿两个硬化组件             | +40%护盾硬化<br/>+40%装甲硬化                                  | -100  |       |
| 缺陷分析仪     | 未定                      | +15%舰船伤害<br/>+20%护盾穿透<br/>+20%装甲穿透                     | -100  |       |
|           |                         |                                                        |       |       |

- #### 起源线专属护盾装甲

| 类型       | 槽位 | 护盾+回复    | 装甲+回复    | 船体+回复    | 修正      | power |
|----------|----|----------|----------|----------|---------|-------|
| 先进灵力结界护盾 | S  | 3600, 18 | 1200, 6  |          | 护盾硬化+2% | -120  |
| 先进灵力结界护盾 | M  | 5400, 27 | 2200, 11 |          | 护盾硬化+4% | -160  |
| 先进灵力结界护盾 | L  | 7200, 36 | 3200, 16 |          | 护盾硬化+8% | -200  |
| 灵力活性零素装甲 | S  |          | 2400, 12 | 1600, 8  | 装甲硬化+2% | -120  |
| 灵力活性零素装甲 | M  |          | 3200, 16 | 2600, 13 | 装甲硬化+4% | -160  |
| 灵力活性零素装甲 | L  |          | 4000, 20 | 3600, 18 | 装甲硬化+8% | -200  |

- #### C槽(如量子去稳器)

- 月时计
    - 图标为怀表
    - 修正：
        - 【The world】
            - 作用半径400，优先级30，友方
            - 舰船射速+300%
            - 舰船亚光速+300%
        - 【The world】
            - 作用半径400，优先级30，敌方
            - 舰船射速-300%
            - 舰船亚光速-300%
    - 动态效果：
        - 交战时6天内将敌方舰船完全时停
        - 如果敌方也装备该组件则不生效
        - 本动态修正每2天修正一次
- 紧闭的恋之瞳
    - 图标为恋之瞳
    - 修正：
        - 本我的解放
            - 作用半径400，优先级10，友方
            - 闪避+20
        - 完全思维控制
            - 作用半径400，优先级10，敌方
            - 命中-20
            - 追踪-20
    - 动态效果：
        - 交战时15天内将敌方舰船命中追踪动态修正为-100%
        - 交战时15天内将己方舰船闪避动态修正为+100%
        - 如果敌方也装备该组件则不生效
        - 本动态修正每3天修正一次
- 觉之瞳
    - 图标为觉之瞳
    - 修正：
        - 觉之读心术
            - 作用半径400，优先级10，友方
            - 命中+30
            - 追踪+30
        - 完美思维控制
            - 作用半径400，优先级10，敌方
            - 闪避-10
    - 动态效果：
        - 交战时15天内将敌方舰船闪避动态修正为-100%
        - 交战时15天内将己方舰船命中追踪动态修正为+100%
        - 如果敌方也装备该组件则不生效
        - 本动态修正每3天修正一次
- 恐怖的谜团-动与静的均衡
    - 图标为隙间
    - 修正：
        - 乖离「无名迷雾」
            - 作用半径400，优先级1，敌方
            - 护盾硬化-100%
            - 装甲硬化-100%
            - 舰船武器伤害-51%
        - 结界「动与静的均衡」
            - 作用半径400，优先级1，敌方
            - 舰船基础速度-51%
            - 舰船武器射速-51%
            - 舰船闪避-51%
        - 「恐怖奏鳴曲」
            - 作用半径400，优先级1，敌方
            - 命中-51%
            - 追踪-51%
            - 护盾穿透-100%
            - 装甲穿透-100%
    - 动态效果：
        - 交战时随机触发下列一种效果，技能冷却17天，对于绝大多数敌人都能生效
        - 乖离「无名迷雾」
            - 使得敌方随机25%~50%艘舰船被大幅度压制7天
            - 触发几率50%
            - 实际效果：
                - 舰船武器伤害动态修正为-90%
                - 抹除每日船体/护盾/装甲回复
                - 硬化和双穿动态修正为-100%
        - 结界「动与静的均衡」
            - 使得敌方随机10%～25%艘舰船被禁锢7天
            - 触发几率40%
            - 实际效果：
                - 舰船被时停
        - 「恐怖奏鳴曲」
            - 使得敌方全部舰船因为船员陷入恐惧而命中大幅度降低且无法闪避，7天后结束
            - 触发几率10%
            - 实际效果：
                - 舰船命中追踪闪避动态修正为-95%
                - 抹除舰船命中追踪闪避加算加成
                - 硬化和双穿动态修正为-100%
        - 如果敌方也装备该组件则不生效
        - 本动态修正每天都会修正一次

#### 计划中组件

- 幽幽子C槽，图标为幽幽子的扇子：作战时每击杀敌方一艘舰船积攒1点亡灵
    - 亡灵达到10时为己方提供少量增益。
    - 亡灵达到25时每5天修复全体舰船。
    - 亡灵达到50时提供大量增益。
    - 亡灵达到75时将在下次交战使对方全体船体值变为原来的1%。
    - 亡灵达到100时每天都会修复舰船。
    - 脱离战斗后每个月减少5点亡灵。
- 舰船传感器，需要美术资源
