-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的 WEB 接口数据库升级到 1.1.19 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- 将原 recruitment 表中的数据转换到 party_bookings
-- 该脚本假定你的 WEB 接口数据库与主数据库是同一个并且已执行主数据库 1.1.19 的升级脚本
-- 执行完成该脚本后进入游戏测试, 若一切正常可以手动移除 recruitment 表

INSERT INTO `party_bookings` (`world_name`, `account_id`, `char_id`, `char_name`, `purpose`, `assist`, `damagedealer`, `healer`, `tanker`, `minimum_level`, `maximum_level`, `comment`) SELECT `world_name`, `account_id`, `char_id`, `char_name`, `adventure_type`, `assist`, `dealer`, `healer`, `tanker`, `min_level`, `max_level`, `memo` FROM `recruitment`;
