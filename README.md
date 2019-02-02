# DpiCmdChanger
Windows tool for chaning DPI value via command line.  

## What is it?
This tool is indented to use for people who are frequently chaning DPI value for their monitor.  
With this tool you can use one command to change DPI, instead of going through Windows Settings app.  
If you want to define hotkey for that operation, please use tools like [AutoHotKey](https://www.autohotkey.com/).

## How to use?
1. Run **```dpi```** with **```list```** argument to obtain available monitors.  
```dpi list```
2. Choose monitor from list and run **```dpi```** with desired DPI level and monitor ID (```LGD047A``` is example monitor ID)  
```dpi -1 LGD047A```
3. If you want to omit specifying monitor ID every time, create **```dpi.cfg```** file next to the **```dpi.exe```** and fill it with monitor ID.
4. Now you can change monitor DPI just by **```dpi level```** (**```level```** is desired DPI level)  
```dpi 1```

## Where to download?
See [Releases](https://github.com/SzymonKatra/DpiCmdChanger/releases)

## How to compile?
Use provided Visual Studio C++ 2017 project file.