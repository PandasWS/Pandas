-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.8 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 若您在 inter_athena.conf 中启用了 use_sql_db 选项, 那么请导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20211118.sql
-- -----------------------------------------------

ALTER TABLE `item_db_re`
	ADD COLUMN `class_fourth` tinyint unsigned DEFAULT NULL
;
ALTER TABLE `item_db2_re`
	ADD COLUMN `class_fourth` tinyint unsigned DEFAULT NULL
;
