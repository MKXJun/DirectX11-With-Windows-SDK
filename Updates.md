更新记录
2018/10/26 v18.5</br>
-修正了项目12反射光照异常的问题

2018/10/25 v18.4</br>
-修正了GameObject类的一点小问题

2018/10/24 v18.3</br>
-顶点/索引缓冲区的Usage改为D3D11_USAGE_IMMUTABLE</br>
-顶点着色器变量名微调</br>
-Vertex.h微调</br>
-d3dUtil.h中的CreateDDSTexture2DArrayShaderResourceView更名为CreateDDSTexture2DArrayFromFile</br>
-考虑到不使用预编译头，所有项目头文件结构大幅调整</br>
-DXTK属性配置表微调</br>

2018/10/20 v18.2</br>
-将BasicFX和BasicObjectFX都更名为BasicEffect</br>
-将文件的.fx后缀改为.hlsli

2018/10/19 v18.1</br>
-优化着色器(减少汇编指令)

2018/10/17 v18.0</br>
-添加项目Picking</br>
-修正d3dUtil.h中HR宏的问题

2018/9/29 v17.1</br>
-添加d3dUtil.h</br>
-添加Camera关于ViewPort的方法</br>
-精简头文件</br>
-修改错误描述

2018/9/25 v17.0</br>
-添加项目 Instancing and Frustum Culling</br>
-删掉了到现在都没什么用的texTransform</br>
-调整注释

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