# 综述

对于Win10系统的大多数用户来说，可以直接编译本教程对应的项目并运行。但也有部分用户由于某些原因可能会出现无法编译的情况。

**[DirectX11 With Windows SDK完整目录](http://www.cnblogs.com/X-Jun/p/9028764.html)**

**欢迎加入QQ群: 727623616 可以一起探讨DX11，以及有什么问题也可以在这里汇报。**

## 与当前项目的Windows SDK版本不一致

对于Visual Studio 2017，你可以点击项目-重定解决方案目标，选择你当前拥有的SDK版本即可。

如果没有上述选项，你需要全选所有项目，并在项目属性-常规中，将Windows SDK版本改为当前你拥有的SDK版本。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/001.png)

## 存在大量的编译错误

**此问题仅针对SDK版本在10.0.162099.0以下，并且是使用VS2017的情况。**

由于在Visual Studio 2017版本15.5及更高版本所创建的新项目中，新增了编译器的标准符合性模式(`/permissive-`)，并且该选项默认是**开启**的。该选项用于检测一些非标准C++语言的写法，并且仅支持从`10.0.162099.0`开始到目前最新版本的Windows SDK。旧版本的Windows SDK在编译时会引发大量的编译错误，需要在项目属性- C/C++ -语言中，将符合模式设为**否**。

>注意: 在低版本的Visual Studio 2017，以及Visual Studio 2015是没有标准符合性模式的设置的，并且默认为**否**。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/002.png)

具体可以参考 [触发-(标准符合性)](https://docs.microsoft.com/zh-cn/cpp/build/reference/permissive-standards-conformance?view=vs-2017)

## Debug模式下打开程序出现D3D11CreateDevice Failed

这种情况下Release模式应该还是可以运行的，现在查看调试输出窗口应该会有如下信息

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/003.png)

目前已经确认是你电脑的Win10系统没有安装图形工具。首先点击Win-Windows 管理工具-服务

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/004.png)

找到服务(本地)中的Windows Update项，如果没有启用，则将它启动。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/005.png)

然后右键Win-设置，搜索：管理可选功能，进去后查看现在可选功能是否包含了图形工具，若没有则添加该功能，安装完成后可以看到：

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/006.png)

现在应该可以进行调试了

## 提示Direct3D Feature Level 11 unsupported

出现这个说明你的显卡不支持特性等级11.0，你可以尝试给特性等级数组添加`D3D_FEATURE_LEVEL_10_1`和`D3D_FEATURE_LEVEL_10_0`，然后将所有的HLSL编译器使用的着色器模型下调至`Shader Model 4.0`，还要在所有`CreateShaderFromFile`函数中下调。

# Windows 7系统无法直接运行的解决方法

在编写该项目的时候一开始是只考虑了Win 10 系统，没有考虑向下兼容的，但现在既然要做到兼容(可能是部分兼容)，还需要在原有的项目基础进行一些额外的配置。

## 无法定位程序输入点CreateFile2于动态链接库kernel32.dll上
`CreateFile2`函数仅Windows 8以上的系统支持，该函数在`DDSTextureLoader`中用到，但我们可以通过修改`_WIN32_WINNT`的值来让它使用`CreateFileW`。

在项目属性-C/C++ -预处理器中按下面的方式添加宏：

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/How-To-Build-Solution/007.png)

然后重新编译解决方案/项目即可。

## 从本教程项目08起都无法看到文字

考虑到不应该只是因为无法看到文字就让程序运行不了，经过修改后，如果你的系统不支持DirectX 11.1，则将不会显示文字。如果你想要在Windows 7系统上看到项目的文字，则需要：
1. 更新Windows 7系统直到安装了Service Pack 1
2. 安装KB2670838补丁

## 缺少d3dCompiler_47.dll

Windows 7系统通常情况下是缺少该动态库的，但如果你装了Visual Studio 2015/2017，通常会包含该动态库供使用。为此，你需要从Visual Studio的安装路径中找到运行库对应的版本，如：
Windows SDK 8.1对应`C:\Program Files (x86)\Windows Kits\8.1\Redist\D3D`，然后再根据x64还是x86将里面的`d3dCompiler_47.dll`复制到你的项目，或者`C:\Windows\System32`中。

## 缺少api-ms-win-core-libraryloader-l1-1-0.dll
出现该问题是因为将不合适的`d3dCompiler_47.dll`拉入到项目或系统环境中，参照上一条进行操作。