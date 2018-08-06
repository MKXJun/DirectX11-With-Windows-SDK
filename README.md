# DirectX11 With Windows SDK

里面的解决方案/项目需要使用Visual Studio 2017来打开，并且需要使用最新的Windows SDK版本(10.0.17134.0)。若要指定哪个项目，需要对项目右键-设为启动项。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/001.jpg)

然后对项目右键-选择生成，或者对整个解决方案进行生成都可以。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/002.jpg)

目前对应的博客更新：

[https://www.cnblogs.com/X-Jun/p/9028764.html](https://www.cnblogs.com/X-Jun/p/9028764.html)（当前最新）

[https://blog.csdn.net/x_jun96/article/details/80293670](https://blog.csdn.net/x_jun96/article/details/80293670)

项目如果有编译不通过的问题的话可以提出来。

# 更新记录
2018/8/6 v11.1</br>
-将所有项目的着色器代码都放到对应的hlsl文件中
-细化Living Without FX11的绘制顺序

2018/8/5 v11.0</br>
-添加项目 Depth Test</br>
-编译二进制着色器文件的后缀名变成*.vso, *.pso, *.hso, *.gso, *.hso, *.cso</br>
-修正了Geometry类中圆柱体的模型问题，并添加了只包含圆柱侧面的方法</br>
-BasicManager类更名为BasicFX类</br>
-一些小修改

2018/7/29 v10.0</br>
-添加项目 Living Without FX11

2018/7/28 v9.0</br>
-添加项目 Depth and Stenciling</br>
-修正了光照、打包问题</br>
-修正了鼠标RELATIVE模式无法使用的问题

2018/7/24 v8.2</br>
-添加.gitignore

2018/7/23 v8.1</br>
-将着色器模型从4.0换成5.0

2018/7/21 v8.0</br>
-添加项目 Blending

2018/7/20 v7.1</br>
-修改了Material结构体构造函数的问题</br>
-每个解决方案将管理10章内容的项目，减小单个解决方案编译的项目数

2018/7/16 v7.0</br>
-添加项目 Camera</br>
-添加DirectXTK源码，删去旧有库，用户需要自己编译

2018/7/14 v6.2</br>
-修改了光照高亮(镜面反射)部分显示问题

2018/7/13 v6.1</br>
-HLSL和纹理分别使用单独的文件夹</br>
-消除x64平台下的一些警告

2018/7/12 v6.0</br>
-原DX11 Without DirectX SDK 现在更名为 DirectX11 With Windows SDK</br>
-添加项目 Texture Mapping

2018/6/21 v5.1</br>
-调整项目配置

2018/5/29 v5.0</br>
-添加项目 Direct2D and Direct3D Interoperability

2018/5/28 v4.0</br>
-添加项目 Lighting

2018/5/21 v3.1</br>
-小幅度问题修改

2018/5/14 v3.0</br>
-添加项目 Mouse and Keyboard</br>
-小幅度问题修改

2018/5/13 v2.0</br>
-添加项目 Rendering a Cube</br>
-添加项目 Rendering a Triangle

2018/5/12 v1.0</br>
-添加项目 DirectX11 Initialization


