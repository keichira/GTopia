--
CREATE TABLE `Bans` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserID` int UNSIGNED DEFAULT 0,
  `BannerID` int UNSIGNED DEFAULT 0,
  `Reason` char(128) DEFAULT '',
  `BannedTime` datetime NOT NULL,
  `ExpireTime` datetime NOT NULL,
  `ModType` int DEFAULT -1,
  `IP` char(15) NOT NULL DEFAULT '0.0.0.0',
  `RID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `PlatformType` smallint NOT NULL DEFAULT '-1',
  `GID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `VID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `Hash` int NOT NULL DEFAULT '0',
  `SID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  PRIMARY KEY (`ID`),
  KEY `idx_gid` (`GID`),
  KEY `idx_rid` (`RID`),
  KEY `idx_ip` (`IP`),
  KEY `idx_vid` (`VID`),
  KEY `idx_hash` (`Hash`),
  KEY `idx_sid` (`SID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

ALTER TABLE Players MODIFY COLUMN ExtraData BLOB DEFAULT NULL;