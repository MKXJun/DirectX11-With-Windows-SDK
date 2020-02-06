# DirectX11 With Windows SDK教程演示项目
[![Build status](https://ci.appveyor.com/api/projects/status/fv2f3emvusqsuj49?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk-hk9xb)[![Build status](https://ci.appveyor.com/api/projects/status/9ntk5efu2h7mkbgn?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk) [![Build status](https://ci.appveyor.com/api/projects/status/dpl8y4uea5cv0303?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk-s5k2l) ![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-1.26.0-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x

## 最近更新

Ver 1.26.0</br>
-添加项目 OIT</br>
-调整`TextureRender::Begin`方法</br>
-调整项目文件布局

## 博客教程

**[博客园（优先保证最新）](https://www.cnblogs.com/X-Jun/p/9028764.html)**

[CSDN（至少每个月同步一次）](https://blog.csdn.net/x_jun96/article/details/80293670)

## QQ群交流

QQ群号：727623616

欢迎大家来交流，项目及博客有什么问题也可以在这里提出。

## 如何运行项目

**对于Win10系统，请根据自己当前的Visual Studio版本打开**

**对于Win7和Win8.x的系统，请选择DirectX11 With Windows SDK(VS2015 Win7).sln打开**

建议一次性生成所有项目，比单独生成会快很多。生成完成后，若要指定运行哪个项目，需要对项目右键-设为启动项。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/001.png)

如遇到项目无法编译、打开的问题，[请点此处](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/README.md)

## 配置表

|IDE            |**VS2019**    |**VS2017**        |VS2015     |
|--------------|:-------------:|:----------------:|:------------:|
|D3DComplier版本|**47**          |**47**           |47         |
|Windows SDK版本|**10.0 (最新安装的版本)**|**10.0.18362.0**  |10.0.14393.0        |
|Windows开发/运行环境 |**Windows 10**|**Windows 10**|Windows 7 SP1及更高版本|
|平台           |**x86/x64支持**      |**x86/x64支持**   |x86/x64支持|
|配置           |**Debug/Release支持**|**Debug/Release支持**|Debug/Release支持|

> **注意：** 
>
> 1. **教程不支持VS2013及更低版本！**
> 2. **VS2015在安装时需要勾选VS2015 更新 3， 以及Tools(1.4.1)和Windows 10 SDK(10.0.14393)！**
> 3. Win7系统需要安装Service Pack 1以及KB2670838补丁以支持Direct3D 11.1
> 4. 建议安装配置表列出的VS所使用的对应版本的Windows SDK

## 支持/赞赏博主
**博客和项目维护不易，如果本系列教程对您有所帮助，希望能够扫码支持一下博主，谢谢！**

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/002.png)![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/003.png)

## 其他

[使用Direct3D 11.x(Windows SDK)编写的魔方](https://github.com/MKXJun/Rubik-Cube)

[完整更新记录](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)