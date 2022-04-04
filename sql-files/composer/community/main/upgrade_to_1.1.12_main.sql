-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.12 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20221103.sql
-- -----------------------------------------------

-- Reset the following quests to match official Rockridge quests
DELETE FROM `quest` WHERE `quest_id` = 1321;
DELETE FROM `quest` WHERE `quest_id` = 1322;
DELETE FROM `quest` WHERE `quest_id` = 1323;
DELETE FROM `quest` WHERE `quest_id` = 1324;
DELETE FROM `quest` WHERE `quest_id` = 1325;
DELETE FROM `quest` WHERE `quest_id` = 1326;
DELETE FROM `quest` WHERE `quest_id` = 1327;
DELETE FROM `quest` WHERE `quest_id` = 1328;
DELETE FROM `quest` WHERE `quest_id` = 1329;
DELETE FROM `quest` WHERE `quest_id` = 1330;
