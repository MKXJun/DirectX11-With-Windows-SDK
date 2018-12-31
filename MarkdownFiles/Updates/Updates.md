更新记录

2018/12/31 v21.11</br>
-修复项目09-14着色器入口点不匹配的问题
-CreateShaderFromFile函数形参名调整

2018/12/24 v21.10</br>
-删除dxErr库，Windows SDK 8.0以上已内置错误码消息映射</br>
-清理无用文件

2018/12/21 v21.9</br>
-修复Win7部分项目在x86下编译失败的问题

2018/12/20 v21.8</br>
-添加Win7配置的项目和解决方案</br>
-项目21 添加鼠标点击物体后的响应

2018/12/15 v21.7</br>
-Geometry现支持多种形式的顶点(除VertexPosSize)</br>
-项目代码大幅度调整，为Geometry进行适配

2018/12/12 v21.6</br>
-修复Model使用Geometry时无法显示的异常

2018/12/11 v21.5</br>
-添加文档帮助用户解决项目无法编译、运行的问题</br>
-代码调整

2018/12/6 v21.4</br>
-对于不支持Direct2D显示的系统，将不显示文本</br>
-修改项目08标题问题</br>
-程序现在都生成到项目路径中

2018/12/4 v21.3</br>
-调整GameTimer</br>
-编译好的着色器文件后缀统一为.cso</br>
-调整部分Effects

2018/11/19 v21.2</br>
-修复项目17异常

2018/11/18 v21.1</br>
-调整D3DApp</br>
-修复项目24异常

2018/11/17 v21.0</br>
-添加项目 Render To Texture</br>
-调整RenderStates的初始化</br>
-调整初始化2D纹理时MipLevels的赋值</br>
-删除以10个为单位的解决方案</br>
-项目SDK版本更新至(17763)

2018/11/9 v20.4</br>
-添加AppVeyor，帮助检测项目编译情况(进行全属性配置编译)</br>
-添加用于检测的解决方案DirectX11 With Windows SDK.sln

2018/11/7 v20.3</br>
-修正第一/三人称视角程序窗口不在中心的问题</br>
-由于Windows SDK 8.1下DirectXMath.h的类构造函数为Trivial，需要人工初始化，如：XMFLOAT3(0.0f, 0.0f, 0.0f)。

2018/11/5 v20.2</br>
-将调用IUnknown::QueryInterface改成ComPtr调用As方法

2018/11/4 v20.1</br>
-修正Geometry类因多个重载支持无法编译通过的问题</br>
-修正项目16无法运行的问题</br>
-**提取了DXTK的键鼠类，并砍掉了DXTK库，现在每个项目可以独自编译了**

2018/11/2 v20.0</br>
-添加项目 Dynamic Cube Mapping</br>
-再次修正Geometry类圆柱显示问题</br>
-修正项目21-22中关于GameObject类忘记转置的问题

2018/10/29 v19.1</br>
-从.fx换成.hlsli后就不需要人工标记"不需要生成"了

2018/10/28 v19.0</br>
-添加项目 Static Cube Mapping</br>
-修正Geometry类问题

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
