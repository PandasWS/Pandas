/*
 Navicat Premium Data Transfer

 Source Server         : localhost
 Source Server Type    : MariaDB
 Source Server Version : 100508
 Source Host           : localhost:3306
 Source Schema         : ragnarok

 Target Server Type    : MariaDB
 Target Server Version : 100508
 File Encoding         : 65001

 Date: 05/05/2022 13:03:49
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for quest_acc
-- ----------------------------
DROP TABLE IF EXISTS `quest_acc`;
CREATE TABLE `quest_acc`  (
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `quest_id` int(10) UNSIGNED NOT NULL,
  `state` enum('0','1','2') CHARACTER SET gbk COLLATE gbk_bin NOT NULL DEFAULT '0',
  `time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `count1` mediumint(8) UNSIGNED NOT NULL DEFAULT 0,
  `count2` mediumint(8) UNSIGNED NOT NULL DEFAULT 0,
  `count3` mediumint(8) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`, `quest_id`) USING BTREE
) ENGINE = MyISAM CHARACTER SET = gbk COLLATE = gbk_bin ROW_FORMAT = Fixed;

SET FOREIGN_KEY_CHECKS = 1;
