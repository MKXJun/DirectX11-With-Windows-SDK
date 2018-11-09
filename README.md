# DirectX11 With Windows SDK系列教程与演示项目
[![Build status](https://ci.appveyor.com/api/projects/status/9ntk5efu2h7mkbgn?svg=true)](https://ci.appveyor.com/project/MKXJun/directx11-with-windows-sdk) ![](https://img.shields.io/badge/license-MIT-000000.svg) ![](https://img.shields.io/badge/Ver-20.4-519dd9.svg)

## 博客教程

目前对应的博客更新：

[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)（优先保证最新）

[CSDN](https://blog.csdn.net/x_jun96/article/details/80293670)

## QQ群交流

QQ群号：727623616

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

## 项目概述

语言:</br>
-C++11和少量C++14</br>
-HLSL Shader Model 5.0


目前所有项目无需依赖第三方库的编译，而是从微软官方项目中提取了下述模块到项目中：</br>
[DirectXTex/DDSTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/DDSTextureLoader)</br>
[DirectXTex/WICTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/WICTextureLoader)</br>
[DXTK/Mouse(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)</br>
[DXTK/Keyboard(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)</br>
[DXUT/dxerr](https://github.com/Microsoft/DXUT/tree/master/Core)</br>

作为教程演示项目，并不是以实现一个软引擎为目标，因此不会刻意进行引擎架构的组织。建议读者在跟随教程学习的同时要动手实践。

## 配置表

|               |默认配置|最低配置|
|---------------|:------------:|:------:|
|IDE            |**VS2017**        |VS2015  |
|D3DComplier版本|**47**            |44      |
|Windows SDK版本|**10.0.17134.0**(支持10.0.16299.0)|8.1|
|Windows开发环境|**Windows 10**    |Windows 8.1|
|平台           |**x86/x64支持**   |x86/x64支持|
|配置           |**Debug/Release支持**|Debug/Release支持|

>注意: 默认版本以下的Windows SDK需要在项目设置-配置属性-C/C++-语言-符合模式 设为否，才能正常编译。


## 如何打开教程项目

**DirectX11 With Windows SDK.sln包含目前所有项目，同时用于编译测试**

**建议选择以10章为1组的解决方案打开**

若要指定哪个项目，需要对项目右键-设为启动项。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/001.png)

然后对项目右键-选择生成，或者对整个解决方案进行生成都可以。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/002.jpg)


## 更新记录

该项目的更新记录位于Updates.md

