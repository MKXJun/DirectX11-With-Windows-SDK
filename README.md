# DirectX11 With Windows SDK

## 基本配置
IDE：Visual Studio 2017 Community
语言：C++/HLSL
Windows SDK版本：10.0.17134.0(最新版本)

项目如果有编译不通过的问题的话可以提Issue。如果是DirectXTK的问题，请到下面链接寻找合适的解决方案尝试编译，若能够通过则替换掉现有的DirectXTK_Desktop_2017_Win10项目，按照博客第4章教程进行配置即可。

**紧急事项(2018/9/3):如果现在DirectXTK无法通过编译，出现了下面的错误：**</font>

**C7510 "Callback": 模板 从属名称的使用必须以"模板"为前缀 event.h**</font>

**在**[**StackOverflow**](https://stackoverflow.com/questions/51864528/update-visual-studio-2017-now-getting-compile-error-c7510-callback-use-of-d)**找到了问题所在**

**双击该错误，首先定位到316行，把:** `return DelegateHelper::Traits::Callback(Details::Forward(callback));` 修改为 `return DelegateHelper::Traits::template Callback(Details::Forward(callback));`

**然后定位到324行，把:** `DelegateHelper::Traits::Callback(` **修改为** `DelegateHelper::Traits::template Callback(`

## 如何打开教程项目
若要指定哪个项目，需要对项目右键-设为启动项。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/001.jpg)

然后对项目右键-选择生成，或者对整个解决方案进行生成都可以。

![](https://github.com/MKXJun/DirectX11-With-Windows-SDK/blob/master/MarkdownFiles/002.jpg)

## 博客教程

目前对应的博客更新：

[博客园](https://www.cnblogs.com/X-Jun/p/9028764.html)（优先保证最新）

[CSDN](https://blog.csdn.net/x_jun96/article/details/80293670)


## 更新记录

2018/9/18 v16.1</br>
-项目13, 14, 15, 16, 17, 19已更新为自己写的Effects框架

2018/9/17 v16.0</br>
-重构项目13，重新编写简易Effects框架</br>
-重新调整一些变量名和函数名，消灭了一些warning</br>
-解决了项目19字符集问题

2018/9/12 v15.1</br>
-修改ObjReader，解决镜像问题

2018/9/11 v15.0</br>
-添加项目 Meshes

2018/8/19 v14.0</br>
-补回4倍多重采样的支持设置</br>
-调整代码细节</br>
-添加项目 Tree Billboard

2018/8/12 v13.1</br>
-解决新项目x64编译无法通过的问题

2018/8/11 v13.0</br>
-添加项目 Stream Output

2018/8/9 v12.1</br>
-修正GameObject类的小错误

2018/8/8 v12.0</br>
-添加项目 Geometry Shader Beginning</br>
-小幅度修改键盘输入和着色器变量名标识问题

2018/8/8 v11.3</br>
-重新调整运行时编译/读取着色器的所有相关代码

2018/8/7 v11.2</br>
-修正项目11-13无法运行的问题

2018/8/6 v11.1</br>
-将所有项目的着色器代码都放到对应的hlsl文件中</br>
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


