# DirectX11 With Windows SDK教程演示项目
[![Build status](https://ci.appveyor.com/api/projects/status/fv2f3emvusqsuj49?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk-hk9xb) ![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-1.33.1-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x

**目前部分项目已支持ImGui**

## 最近更新

2022/3/4 Ver1.33.1

- 修正项目36 法线图显示错误的问题

## 博客教程

目前对应的博客更新：

**[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)（优先保证最新）**

[CSDN](https://blog.csdn.net/x_jun96/article/details/80293670)

## QQ群交流

QQ群号：727623616

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

## 项目概况

语言:</br>
- C++14</br>
- HLSL Shader Model 5.0

目前项目添加了下述代码库或文件：
- [ocornut/imgui](https://github.com/ocornut/imgui)：当前已经为这些项目使用ImGui：第7、10、15、16、17、20、23、30-36章。</br>
- [nothings/stb](https://github.com/nothings/stb)：使用其stb_image</br>
- [tinyobjloader/tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)</br>
- [DirectXTex/DDSTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/DDSTextureLoader)</br>
- [DirectXTex/WICTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/WICTextureLoader)</br>
- [DirectXTex/ScreenGrab](https://github.com/Microsoft/DirectXTex/tree/master/ScreenGrab)</br>
- [DirectXTK/Mouse(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)：不能和imgui同时使用</br>
- [DirectXTK/Keyboard(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)：不能和imgui同时使用</br>

作为教程演示项目，这里并不是以实现一个软引擎为目标。建议读者在跟随教程学习的同时要动手实践。

## 如何打开教程项目

**对于Win10系统，直接打开DirectX11 With Windows SDK(2019 Win10).sln**

**对于Win7和Win8.x的系统，请使用cmake构建项目**

可使用cmake-gui.exe构建项目，其中Win 7系统的用户需要勾选`WIN7_SYSTEM_SUPPORT`。

若取消勾选`USE_IMGUI`，则使用的是原来Direct2D和DWrite作为UI的版本。

建议一次性生成所有项目，比单独生成会快很多。生成完成后，若要指定运行哪个项目，需要对项目右键-设为启动项。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/raw/master/MarkdownFiles/001.png)

> **注意：** 
> 1. **教程不支持VS2013及更低版本！**
> 2. **VS2015在安装时需要勾选VS2015 更新 3， 以及Tools(1.4.1)和Windows 10 SDK(10.0.14393)！**
> 3. Win7系统需要安装Service Pack 1以及KB2670838补丁以支持Direct3D 11.1
> 4. 建议安装配置表列出的VS所使用的对应版本的Windows SDK

## 支持/赞赏博主
**博客和项目维护不易，如果本系列教程对您有所帮助，希望能够扫码支持一下博主。**

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/002.png)![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/003.png)

## 遇到项目无法编译、运行的问题
**[点此查看无法编译、运行教程项目的解决方法](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/README.md)**

## 使用Direct3D 11.x(Windows SDK)编写的魔方

**[点此查看](https://github.com/MKXJun/Rubik-Cube)**

## 更新记录

**[点此查看](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)**

