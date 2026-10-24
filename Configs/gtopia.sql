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
  `ExtraData` BLOB DEFAULT NULL,
  `PlayMods` VARBINARY(512) DEFAULT NULL,
  `PlayTimeSeconds` int UNSIGNED NOT NULL DEFAULT '0',
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
  `UserID` int UNSIGNED DEFAULT '0',
  `BannerID` int UNSIGNED DEFAULT '0',
  `Reason` char(255) DEFAULT '',
  `BannedTime` datetime NOT NULL,
  `ExpireTime` datetime NOT NULL,
  `ModType` int DEFAULT '0',
  `PunishType` int DEFAULT '0',
  `IP` char(15) NOT NULL DEFAULT '0.0.0.0',
  `RID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `PlatformType` smallint NOT NULL DEFAULT '-1',
  `GID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `VID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `Hash` int NOT NULL DEFAULT '0',
  `SID` binary(16) NOT NULL DEFAULT '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
  `Mac` char(24) DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `idx_gid` (`GID`),
  KEY `idx_rid` (`RID`),
  KEY `idx_ip` (`IP`),
  KEY `idx_vid` (`VID`),
  KEY `idx_hash` (`Hash`),
  KEY `idx_sid` (`SID`),
  KEY `idx_mac` (`Mac`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `Holidays`
--

CREATE TABLE `Holidays` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `HolidayKey` varchar(64) NOT NULL,
  `DurationDays` int UNSIGNED NOT NULL DEFAULT '1',
  `Type` tinyint NOT NULL DEFAULT '0',
  `StartTimeUnix` int UNSIGNED NOT NULL DEFAULT '0',
  `IsActive` tinyint NOT NULL DEFAULT '0',
  `ScheduleType` tinyint NOT NULL DEFAULT '0',
  `ScheduleParam` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `HolidayKey` (`HolidayKey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `HolidayRules`
--

CREATE TABLE `HolidayRules` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `HolidayKey` varchar(64) NOT NULL,
  `RuleType` tinyint NOT NULL DEFAULT '0',
  `StatKey` varchar(64) NOT NULL,
  `OperatorType` tinyint NOT NULL DEFAULT '0',
  `Threshold` int NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Table structure for table `Stats`
--

CREATE TABLE `Stats` (
  `StatKey` varchar(64) NOT NULL,
  `StringVal` varchar(255) DEFAULT NULL,
  `FloatVal` float DEFAULT '0',
  `IntVal` int DEFAULT '0',
  `UpdatedAt` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `Category` tinyint NOT NULL DEFAULT '0',
  PRIMARY KEY (`StatKey`),
  KEY `idx_category` (`Category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;