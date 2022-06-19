# DirectX11 With Windows SDK教程演示项目

![](https://img.shields.io/badge/license-MIT-dddd00.svg) [![](https://img.shields.io/badge/Ver-2.37.0-519dd9.svg)](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/Updates/Updates.md)

**现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x**

![000](MarkdownFiles/000.png)

>  **注意：2.x.x和1.x.x的主要区别在于19章之后的代码有大幅修改。**

## 博客教程

- [**Github在线版（优先保证最新）**](https://mkxjun.github.io/DirectX11-With-Windows-SDK-Book) 

- **[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)**

**CSDN目前停更**

## QQ群交流

**QQ群号：727623616**

欢迎大家来交流，以及项目有什么问题也可以在这里提出。

## CMake构建项目

使用`cmake-gui.exe`填写源码路径和构建路径，然后只需要关注下面一个变量：

![004](MarkdownFiles/004.png)

- `WIN_SYSTEM_SUPPORT`：默认关闭，仅Win7用户需要勾选

然后就可以点`Generate`生成项目，生成的解决方案位于build文件夹内，或者点`Open Project`打开

## 打开教程项目

> 注意：默认提供的项目会在后续更新中考虑去除。

打开CMake生成的项目，建议切换成**Release x64**。若要指定运行哪个项目，需要对项目右键-设为启动项。然后就可以生成并运行了

![](MarkdownFiles/001.png)

> **注意：** 
> 1. **目前教程仅支持VS2017(或平台工具集v141)及更高版本！**
> 2. Win7打开需要安装Service Pack 1以及KB2670838补丁，但目前更推荐使用ImGui

## 项目概况

语言:</br>
- C++17</br>
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

**2022/6/19 Ver2.37.0**

- 第9章使用ImGui
- 替换带后缀11的DDS/WICTextureLoader和ScreenGrab
- 删除VS项目，现在用户需要使用cmake生成
- 添加VS项目自动生成检查
- HLSL代码统一使用UTF-8 NO BOM（带BOM会导致编译出错，尽管fxc要求ansi编码）

**19章起的改动**

- 代码重新分类为三个文件夹，且19章开始使用统一的Common代码来避免重复
- **统一使用Assimp加载模型**
- 使用ModelManager和TextureManager管理资源，避免重复重建
- **需要使用C++17**
- **back buffer默认使用sRGB格式**，因此不能直接copy渲染结果到交换链，而是要以render的方式写入
- **统一使用EffectHelper**，基于IEffect继承来管理特效资源，承接模型材质和几何数据
- 使用Material存储材质信息、MeshData管理存储在GPU的模型信息
- 修复EffectHelper中OM设置RTV和UAV的部分
- 修改Texture2D、Buffer部分，便于和着色器对应
- Geometry::MeshData更改为GeometryData，避免与MeshData同名
- 具体特效会根据当前使用的Pass和输入的MeshData来获取管线需要在IA阶段绑定的信息
- shader进行精简与部分重写
- 修复切线变换错误的问题
- 去除SkyRender、TextureRender等，以及`CreateWICTexture2DCubeFromFile`等函数，简化天空盒读取流程
- 修正SSAO中shader变换投影纹理坐标错误
- 31章起的项目会缓存编译好的着色器二进制信息，若要重新编译则删掉缓存或者设置`EffectHelper::SetBinaryCacheDirectory`
- 后处理特效绝大部分统一使用全屏三角形渲染然后指定视口的方式

**[历史更新记录](MarkdownFiles/Updates/Updates.md)**