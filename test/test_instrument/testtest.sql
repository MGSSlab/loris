CREATE TABLE `testtest` (
`CommentID` varchar(255) NOT NULL default '',
`UserID` varchar(255) default NULL,
`Examiner` varchar(255) default NULL,
`Testdate` timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
`testCheckbox` varchar(255) default NULL,
`testText` varchar(255) default NULL,
`consent` enum('yes','no') default NULL,
PRIMARY KEY  (`CommentID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
