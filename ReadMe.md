# Using Vulnerability Driver(CorMem.sys)

Privilege escalation was successfully achieved using the CorMem.sys(Physical Memory Read/Write) vulnerable driver, as shown in the figure below.

![image](./pics/ScreenShot_2026-05-15_194342_118.png)


Add BiosToolCommonDriver.sys

Privilege escalation
![image](./pics/ScreenShot_2026-05-17_143556_434.png)


PPL

![image](./pics/ScreenShot_2026-05-17_143717_220.png)


Kill Process
```
DriverLoader.exe <kill processid>
```
开启核晶的*60
![image](./pics/360hejing_2026-05-26_160354_877.png)

![image](./pics/kill%20360_2026-05-26_160557_313.png)

* 新增了VirtualToPhysical的函数
**代码来源于redteamfortress（https://github.com/redteamfortress）的项目PPLShade（https://github.com/redteamfortress/PPLShade）**

**The code is derived from the PPLShade project（https://github.com/redteamfortress/PPLShade）, which is authored by redteamfortress（https://github.com/redteamfortress）. Further information on this project can be found on the redteamfortress GitHub page.**


Loader mapping driver
![image](./pics/LoaderMappingDriver_2026-06-01_123521_640.png)

**代码已经更新**



add commandline 


![image](./pics/mapping%20with%20cmdline.png)

![image](./pics/Privilege%20escaption%20with%20cmdline.png)


dmp lsass (不支持Windows 11新版本)

build
![image](./pics/Tset%20build_2026-06-05_192042_837.png)

remove process protect 
![image](./pics/remove%20lsass%20protection_2026-06-05_190608_025.png)

dmp file  
![image](./pics/dmp%20file_2026-06-05_191338_890.png)  


**Update DriverSelector**  
**Just switch the BYOVD driver you want to use in the DriverSelector file.**   
只要在文件DriverSelector切换你想使用的漏洞驱动即可，注意相应的类型。  


## Syscall
使用了[SysWhispers4](https://github.com/JoasASantos/SysWhispers4)加入到了本项目

Special thanks to the author of [SysWhispers4](https://github.com/JoasASantos/SysWhispers4) for sharing this project.

## BYOVD

**Killer**


| DeviceName | SHA256 | IOCTL CODE |
| :---: | :---: | :---: |
| ardrv| 07c5209bf83065fe760f4fee4ed2308b0c523671f68ca73a3854c2c8c28c0541 | 0x2420031 |
| BootRepair| 5ab36c116767eaae53a466fbc2dae7cfd608ed77721f65e83312037fbd57c946 | 0x222014 |
| ProcessCtr| d64eeb940daffdc8327fb18b160c20e539088cf8407813655f59efa9fdf0022e | 0x89DB202C |
| GGProtect64| 0aa69aee93c6be9bc82680a7df99c114591038ae02e6666fc6e42acb09643111 | 0x223C04 |