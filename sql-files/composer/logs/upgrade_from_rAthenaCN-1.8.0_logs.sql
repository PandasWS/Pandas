
-- upgrade_20180705_logs.sql

ALTER TABLE `picklog`
	MODIFY COLUMN `type` enum('M','P','L','T','V','S','N','C','A','R','G','E','B','O','I','X','D','U','$','F','Y','Z','Q') NOT NULL default 'P';

-- upgrade_20180729_logs.sql

ALTER TABLE `picklog`
	MODIFY COLUMN `type` enum('M','P','L','T','V','S','N','C','A','R','G','E','B','O','I','X','D','U','$','F','Y','Z','Q','H') NOT NULL default 'P';
