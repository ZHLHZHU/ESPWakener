## 简介
EspWakener是一个基于Wake-on-LAN协议的网络唤醒器，通过MQTT协议进行控制。

## 配置说明
* 把开关拨到"ON"，通电（顺序不能乱）。连上Wakener发出的热点，进入192.168.8.1（你都看到本页面了，肯定已经完成这些步骤）。
* 进入Config页签，配置好WIFI名和密码、WOL的端口（默认9）。
* 打开MQTT开关，填入MQTT连接（只支持mqtt://协议，不支持TLS和WS），填入Topic。MQTT用户名和密码如果没有可以忽略。
* 点击保存，把开关拨到"1"，断电，通电。（拨开关和断电的顺序任意，因为本设备只会根据通电瞬间的开关状态来判断是否进入配置模式）。
## 协议
### Wakener上线
当Wakener成功连接Mqtt服务器后，会广播一条自身状态的信息
```json
{
    "action": "join",
    "sender": "SwingFrogWakener",
    "deviceName": "SwingFrogWakener",
    "ip": "192.168.1.188",
    "ssid": "WIFI_NAME",
    "rssi": -47,
    "version": "2.0.0"
}
```
### 请求唤醒
向Wakener所配置的Topic发送以下指令，Wakener将会在所在局域网广播唤醒包
MAC地址分隔符支持 ':' 和 '-'
```json
{
    "action": "wake",
    "mac": "a9:a9:59:30:d8:ee"
}
```
### 请求设备状态
向Wakener所配置的Topic发送以下指令，将会返回Wakener当前状态信息
```json
{
    "action": "info"
}
```
