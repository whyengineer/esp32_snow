# This is the 3d show demo based on MPU6050

## Flow

* the esp32 actes as a websocket server
* the esp32 caculte the euler angle and push the data to the web page
* the web page receive these data and render euler angle in the three.js
## How to use
* the esp32 as a sta mode(you also can use ap mode)
* keep your web browser(chorme test ok) in the same net(or join the esp32 ap)
* open the 3d_show.html use your browser,entering the esp32 ip address and connect it.
![3d_show](https://img.whyengineer.com/3d_show.png) 

# It's working!
