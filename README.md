# DirectX11 With Windows SDK教程演示项目

![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-1.37.1-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x

**目前部分项目已支持ImGui**

## 最近更新

2022/5/27 Ver1.37.1

- cmake现支持assimp，不需要额外配置，最低要求cmake 3.14
- 项目36-40删除VS项目文件，请使用cmake生成项目

**[历史更新记录](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)**

## 博客教程

目前对应的博客更新：

[**Github在线版（优先保证最新）**](https://mkxjun.github.io/DirectX11-With-Windows-SDK-Book) 

**[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)**

**因更新麻烦，CSDN目前考虑停更**

## QQ群交流

QQ群号：727623616

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

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

## 下载教程项目

在命令行，选择合适的位置，然后使用git执行如下命令：

```
git clone --recursive https://github.com/MKXJun/DirectX11-With-Windows-SDK
```

这样就会连同assimp一起克隆到本地

如果assimp文件夹是空的，你也可以去[assimp官网](https://github.com/assimp/assimp)下载最新的release版本，解压后将里面的文件放入assimp文件夹内，并确保CMakeLists.txt暴露在assimp文件夹内

如果不去官网下载的话，cmake也会尝试去下载，但网络不行的话会在这一步卡住。可以去Q群获取项目源码和assimp。

## CMake构建项目

首先需要安装Assimp，**[点此查看](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Assimp/README.md)**

安装好后，使用`cmake-gui.exe`填写源码路径和构建路径，然后只需要关注下面两个变量：

![004](file://E:/Code/C++/DirectX11%20With%20Windows%20SDK/MarkdownFiles/004.png?lastModify=1653587311)

- `USE_IMGUI`：默认开启，关闭后35之前的部分项目使用Direct2D的UI
- `WIN_SYSTEM_SUPPORT`：默认关闭，仅Win7用户需要勾选，但建议保持`USE_IMGUI`开启

然后就可以点`Generate`生成项目，生成的解决方案位于build文件夹内，或者点`Open Project`打开

## 打开教程项目

现在默认给的sln只包含前35章的项目，且只支持VS2017-2022打开。36章及之后的项目需要使用cmake生成。

建议使用**Debug x64或Release x64**的属性配置来生成项目，**Release x64会更快**。生成完成后，若要指定运行哪个项目，需要对项目右键-设为启动项。

![](MarkdownFiles/001.png)

> **注意：** 
> 1. **目前教程仅支持VS2017(平台工具集v141)及更高版本！**
> 2. 如果需要使用Direct2D/DWrite，Win7系统需要安装Service Pack 1以及KB2670838补丁，但目前更推荐使用ImGui



## 创建自己的项目

如果有想要把源码copy到自己的VS项目，又或者是想在VSCode来编写的，**[点此查看](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Your-Project/README.md)**

## 支持/赞赏博主
**博客和项目维护不易，如果本系列教程对您有所帮助，希望能够扫码支持一下博主。**

![](MarkdownFiles/002.png)![](MarkdownFiles/003.png)

## 遇到项目无法编译、运行的问题
**[点此查看无法编译、运行教程项目的解决方法](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/README.md)**

## 使用Direct3D 11.x(Windows SDK)编写的魔方

**[点此查看](https://github.com/MKXJun/Rubik-Cube)**

