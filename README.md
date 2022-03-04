# DirectX11 With Windows SDK教程演示项目
[![Build status](https://ci.appveyor.com/api/projects/status/fv2f3emvusqsuj49?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk-hk9xb) ![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-1.33.0-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x

## 最近更新

2022/3/4 Ver1.33.0

- 添加项目 Deferred Shading Beginning
- 添加Common文件夹存放公共部分代码，但目前仅限36章项目使用
- 添加stb、tiny_obj_loader到Common文件夹中

## 博客教程

目前对应的博客更新：

**[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)（优先保证最新）**

[CSDN](https://blog.csdn.net/x_jun96/article/details/80293670)

## QQ群交流

QQ群号：727623616

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

## 使用第三方库

- **[ocornut/imgui](https://github.com/ocornut/imgui)：**当前已经为这些项目使用ImGui：第7、10、15、16、17、20、23、30-36章。
- **[nothings/stb](https://github.com/nothings/stb)**
- **[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)**


## 如何运行项目

语言:</br>
-C++11和少量C++14</br>
-HLSL Shader Model 5.0

目前所有项目无需依赖第三方库的编译，而是从微软官方项目中提取了下述模块到项目中：</br>
[DirectXTex/DDSTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/DDSTextureLoader)</br>
[DirectXTex/WICTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/WICTextureLoader)</br>
[DirectXTex/ScreenGrab](https://github.com/Microsoft/DirectXTex/tree/master/ScreenGrab)</br>
[DXTK/Mouse(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)</br>
[DXTK/Keyboard(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)</br>

作为教程演示项目，这里并不是以实现一个软引擎为目标。建议读者在跟随教程学习的同时要动手实践。

## 如何打开教程项目

**对于Win10系统，请根据自己当前的Visual Studio版本打开**

**对于Win7和Win8.x的系统，请选择DirectX11 With Windows SDK(Win7).sln打开**

可使用cmake-gui.exe构建项目，其中Win 7系统的用户需要勾选`WIN7_SYSTEM_SUPPORT`。

若取消勾选`USE_IMGUI`，则使用的是原来Direct2D和DWrite作为UI的版本。

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

## 一些有用的代码模块

**[点此查看](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/Modules/)**

## 使用Direct3D 11.x(Windows SDK)编写的魔方

**[点此查看](https://github.com/MKXJun/Rubik-Cube)**

## 更新记录

**[点此查看](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)**

