CREATE TABLE `frem_access_info` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `PIN` char(8) DEFAULT NULL,
  `FREQ` int(11) DEFAULT NULL,
  `SNR` int(11) DEFAULT NULL,
  `RESERVED` char(6) DEFAULT NULL,
  `BS_IP` char(16) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
