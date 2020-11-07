-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 导入此脚本之后, 若您在 inter_athena.conf 启用了 use_sql_db 选项,
-- 那么还需额外导入 upgrade_to_1.0.7_main_use_sql_db.sql 文件
-- ------------------------------------------------------------------------------

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
