-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.8 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- 熊猫模拟器优化对极端计算的支持 (AKA: 变态服拓展包)
-- 因此部分字段与 rAthena 官方设计相比需要调整数据库字段的类型

ALTER TABLE  `char` MODIFY `str` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `char` MODIFY `agi` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `char` MODIFY `vit` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `char` MODIFY `int` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `char` MODIFY `dex` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `char` MODIFY `luk` int(11) UNSIGNED NOT NULL DEFAULT '0';

ALTER TABLE  `homunculus` MODIFY `str` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `homunculus` MODIFY `agi` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `homunculus` MODIFY `vit` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `homunculus` MODIFY `int` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `homunculus` MODIFY `dex` int(11) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE  `homunculus` MODIFY `luk` int(11) UNSIGNED NOT NULL DEFAULT '0';
