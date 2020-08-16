-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.6 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20200327.sql
-- -----------------------------------------------

DROP TABLE `interreg`;

-- -----------------------------------------------
-- upgrade_20200402.sql
-- -----------------------------------------------

-- AB_EUCHARISTICA
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE `s`.id = 2049 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` WHERE `id` = 2049;

-- GN_SLINGITEM
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE `s`.id = 2493 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` WHERE `id` = 2493;

-- GN_MAKEBOMB
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE `s`.id = 2496 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` WHERE `id` = 2496;

-- ONLY RUN THE BELOW QUERIES IF YOU ARE ON RENEWAL
-- CR_CULTIVATION
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE `s`.id = 491 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` WHERE `id` = 491;

-- -----------------------------------------------
-- upgrade_20200506.sql
-- -----------------------------------------------

-- HT_SANDMAN
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE (`c`.class = 4190 OR `c`.class = 4191) AND `s`.id = 119 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` USING `skill`, `char` WHERE (`char`.class = 4190 OR `char`.class = 4191) AND `skill`.id = 119 AND `char`.char_id = `skill`.char_id;

-- HT_FLASHER
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE (`c`.class = 4190 OR `c`.class = 4191) AND `s`.id = 120 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` USING `skill`, `char` WHERE (`char`.class = 4190 OR `char`.class = 4191) AND `skill`.id = 120 AND `char`.char_id = `skill`.char_id;

-- HT_FREEZINGTRAP
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE (`c`.class = 4190 OR `c`.class = 4191) AND `s`.id = 121 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` USING `skill`, `char` WHERE (`char`.class = 4190 OR `char`.class = 4191) AND `skill`.id = 121 AND `char`.char_id = `skill`.char_id;

-- upgrade_20200518.sql

-- WM_DOMINION_IMPULSE
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + `s`.lv WHERE `s`.id = 2417 AND `c`.char_id = `s`.char_id;
DELETE FROM `skill` WHERE `id` = 2417;
