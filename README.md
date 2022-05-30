# DirectX11 With Windows SDK教程演示项目

![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-1.37.2-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

**现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x**

![000](MarkdownFiles/000.png)

## 博客教程

- [**Github在线版（优先保证最新）**](https://mkxjun.github.io/DirectX11-With-Windows-SDK-Book) 

- **[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)**

**CSDN目前停更**

## QQ群交流

**QQ群号：727623616**

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

## 获取教程项目源码

[**点此获取，注意不要选择Source code**](https://github.com/MKXJun/DirectX11-With-Windows-SDK/releases)

下载不了也可以进Q群获取

## CMake构建项目

使用`cmake-gui.exe`填写源码路径和构建路径，然后只需要关注下面两个变量：

![004](MarkdownFiles/004.png)

- `USE_IMGUI`：默认开启，关闭后35之前的部分项目使用Direct2D的UI
- `WIN_SYSTEM_SUPPORT`：默认关闭，仅Win7用户需要勾选，但建议保持`USE_IMGUI`开启

然后就可以点`Generate`生成项目，生成的解决方案位于build文件夹内，或者点`Open Project`打开

## 打开教程项目

如果是从`DirectX11 With Windows SDK(2022 Win10).sln`打开的，现在只包含前35章的项目，且只支持VS2017-2022打开。

从CMake生成的解决方案则包含36章之后的项目。

建议使用**Release x64**。生成完成后，若要指定运行哪个项目，需要对项目右键-设为启动项。

![](MarkdownFiles/001.png)

> **注意：** 
> 1. **目前教程仅支持VS2017(或平台工具集v141)及更高版本！**
> 2. 如果需要使用Direct2D/DWrite，Win7系统需要安装Service Pack 1以及KB2670838补丁，但目前更推荐使用ImGui

## 项目概况

语言:</br>
- C++14/17</br>
- HLSL Shader Model 5.0

目前项目使用了下述代码库或文件：
- [ocornut/imgui](https://github.com/ocornut/imgui)：当前已经为这些项目使用ImGui：第7、10、15、16、17、20、23、30-39章。</br>
- [nothings/stb](https://github.com/nothings/stb)：使用其stb_image</br>
- [assimp/assimp](https://github.com/assimp/assimp)：模型加载</br>
- [DirectXTex/DDSTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/DDSTextureLoader)</br>
- [DirectXTex/WICTextureLoader](https://github.com/Microsoft/DirectXTex/tree/master/WICTextureLoader)</br>
- [DirectXTex/ScreenGrab](https://github.com/Microsoft/DirectXTex/tree/master/ScreenGrab)</br>
- [DirectXTK/Mouse(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)：不能和imgui同时使用</br>
- [DirectXTK/Keyboard(源码上有所修改)](https://github.com/Microsoft/DirectXTK/tree/master/Src)：不能和imgui同时使用</br>

作为教程演示项目，这里并不是以实现一个软引擎为目标。建议读者在跟随教程学习的同时要动手实践。

## 支持/赞赏博主
**博客和项目维护不易，如果本系列教程对您有所帮助，希望能够扫码支持一下博主。**

![](MarkdownFiles/002.png)![](MarkdownFiles/003.png)

## 使用Direct3D 11.x(Windows SDK)编写的魔方

**[点此查看](https://github.com/MKXJun/Rubik-Cube)**

## 最近更新

2022/5/30 Ver1.37.2

- assimp的cmake编译从静态库改回动态库
- 调整项目FXAA

**[历史更新记录](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)**
