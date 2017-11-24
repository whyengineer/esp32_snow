[WhyEngineer](https://www.whyengineer.com) ESP32 SNOW
====

# HardWare:
![esp32_snow](https://img.whyengineer.com/esp32_snow.png?imageView2/2/w/400/h/400/q/75|imageslim) 

* CPU:Xtensa Dual-core 32-bit LX6 microprocessor(s),up to 600 DMIPS
* RAM:4MB(external)+520K(internal)
* ROM:4MB(external)+448(internal)
* WM8978:mclk from gpio0,48k 32bit 2channel
* MPU6050:3-Axi accle and 3-Axi gyro
* BQ24075:Li-ion Charge and power path manage
* Expand all gpio 

# SoftWare Component:
* a based bsp 
* a webserver framework(it's easy to use)
* a websocket-server
* mp3 deocde(helix and mad)
* lightweight http client
* a simple PI(D) algorithm to fuison accel and gyro to euler and quaternion
## ftp-server(about sd card)
![ftp_server](https://img.whyengineer.com/data/ftp%E6%B5%8B%E8%AF%95%E5%9B%BE%E7%89%87.png) 

# Demo:
* webradio
* bluetooth audio
* webcontrol
* baidu_rest_api_rsa
* ble gateway
* 3d show and control

# Todo List:

* a perfect project config(use make menuconfig)
* apple home assistant
* such as qplay protocol
* other funning things.

