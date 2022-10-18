-- ------------------------------------------------------------------------------
-- 此脚本仅用于将 rAthenaCN 1.8.0 的主数据库升级到 Pandas 1.0.0 版本
-- 请先导入 upgrade_from_rAthenaCN-1.8.0_main.sql 文件
-- 若您在 rAthenaCN 的 inter_athena.conf 启用了 use_sql_db 选项,
-- 使用到了 SQL 版本的道具、魔物数据库, 那么请紧接着导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20190628.sql
-- -----------------------------------------------

ALTER TABLE `mob_db`
	MODIFY `Sprite` varchar(24) NOT NULL,
	ADD UNIQUE KEY (`Sprite`)
;

ALTER TABLE `mob_db_re`
	MODIFY `Sprite` varchar(24) NOT NULL,
	ADD UNIQUE KEY (`Sprite`)
;

ALTER TABLE `mob_db2`
	MODIFY `Sprite` varchar(24) NOT NULL,
	ADD UNIQUE KEY (`Sprite`)
;

ALTER TABLE `mob_db2_re`
	MODIFY `Sprite` varchar(24) NOT NULL,
	ADD UNIQUE KEY (`Sprite`)
;
