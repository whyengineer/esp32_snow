#ifndef FTP_D_H
#define FTP_D_H

#ifdef CONFIG_FTP_SERVER

void ftpd_start();

#else 

#error "please use make menuconfig->ESP32_SNOW Config to enable ftp server"

#endif

#endif