--
-- Table structure for table `Players`
--

CREATE TABLE `Players` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `GuestName` varchar(32) NOT NULL,
  `CreationDate` date DEFAULT NULL,
  `Mac` char(24) DEFAULT NULL,
  `GuestID` int DEFAULT '0',
  `IP` char(15) NOT NULL DEFAULT '0.0.0.0',
  `Flags` int NOT NULL DEFAULT '0',
  `Name` varchar(32) DEFAULT '',
  `Password` binary(16) DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `LastSeenTime` datetime DEFAULT NULL,
  `RID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `PlatformType` smallint NOT NULL DEFAULT '-1',
  `GID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `Inventory` varbinary(512) DEFAULT NULL,
  `RoleID` int DEFAULT NULL,
  `VID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `Hash` int NOT NULL DEFAULT '0',
  `SID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `SkinColor` int UNSIGNED NOT NULL DEFAULT '0',
  `Gems` int NOT NULL DEFAULT '0',
  `ProgressData` varbinary(512) DEFAULT NULL,
  `LastWorld` int NOT NULL DEFAULT '0',
  `ExtraData` varbinary(2048) DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `idx_gid` (`GID`),
  KEY `idx_rid` (`RID`),
  KEY `idx_ip` (`IP`),
  KEY `idx_vid` (`VID`),
  KEY `idx_hash` (`Hash`),
  KEY `idx_sid` (`SID`),
  KEY `idx_name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `Worlds`
--

CREATE TABLE `Worlds` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `Name` varchar(64) NOT NULL,
  `Flags` int NOT NULL DEFAULT '0',
  `LastSeenTime` datetime DEFAULT NULL,
  `Version` int NOT NULL DEFAULT '0',
  `CreationDate` date DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `idx_name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `SchemaMigrations`
--

CREATE TABLE `SchemaMigrations` (
  `Version` VARCHAR(255) PRIMARY KEY,
  `ApplyTime` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `Bans`
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