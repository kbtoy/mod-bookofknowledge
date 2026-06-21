-- Remove the old, hardcoded table from the original abandoned module
DROP TABLE IF EXISTS `attriboost_attributes`;

-- Create our new, infinitely scalable table
CREATE TABLE IF NOT EXISTS `character_bookofknowledge` (
  `guid` int(10) unsigned NOT NULL COMMENT 'Player GUID',
  `stat_type` tinyint(3) unsigned NOT NULL COMMENT '0=Banked Points, 1=Block, 10=Strength, etc.',
  `amount` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Total points allocated',
  PRIMARY KEY (`guid`,`stat_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;