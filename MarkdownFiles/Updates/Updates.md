# 更新记录

大版本号 固定</br>
中版本号 项目添加</br>
小版本号 项目调整(随中版本号添加归零)

2019/3/17 v1.24.4</br>
-修正项目07, 08异常

2019/3/13 v1.24.3</br>
-修正FOV不统一问题(后期统一成pi/3)

2019/3/11 v1.24.2</br>
-修正项目07异常

2019/3/5 v1.24.1</br>
-简化项目26和27，去掉大量无用的方法和成员

2019/2/19 v1.24.0</br>
-**添加项目27 Bitonic Sort**</br>
-Mouse和Keyboard类添加包含Windows.h

2019/2/16 v1.23.5</br>
-修正平面镜反射的光照

2019/2/12 v1.23.4</br>
-修正一些代码小错误</br>
-文件编码恢复

2019/2/11 v1.23.3</br>
-修正SkyRender中读取纹理时创建mipmap的问题</br>
-d3dUtil.h添加缓冲区函数</br>
-修正Collision中立方体线框创建问题

2019/2/6 v1.23.2</br>
-源代码文件编码格式修改为UTF-8 BOM(不需要考虑跨平台)</br>
-HLSL相关文件编码格式修改为UTF-8(fxc编译hlsl不支持UTF-8 BOM)

2019/2/6 v1.23.1</br>
-常量缓冲区USAGE改为DYNAMIC

2019/1/31 v1.23.0</br>
-**添加项目26 Compute Shader Beginning**</br>
-调整项目10 摄像机切换时的初始视角

2019/1/27 v1.22.3</br>
-调整注释</br>
-移除BasicEffect::SetWorldViewProjMatrix方法</br>
-更新教程无法编译、运行的方法

2019/1/14 v1.22.2</br>
-移除d3dUtil、EffectHelper对DXTrace的依赖</br>
-修改函数`CreateWICTextureCubeFromFile`为`CreateWICTexture2DCubeFromFile`</br>
-修复天空盒无法创建mipmaps的问题

2019/1/9 v1.22.1</br>
-修正第三人称摄像机的旋转</br>
-第三人称摄像机添加方法SetRotationX和SetRotationY</br>
-修正某些BasicEffect常量缓冲区标记更新索引不一致问题

2019/1/6 v1.22.0</br>
-**添加项目25 Normal Mapping**</br>
-添加DXTrace</br>
-调整mbo格式以及物体材质</br>
-添加Modules文件夹，存放可复用的模块

2019/1/2 v1.21.12</br>
-所有的项目默认开启4倍多重采样</br>
-添加、更新著作权年份与内容

2018/12/31 v1.21.11</br>
-修复项目09-14着色器入口点不匹配的问题</br>
-CreateShaderFromFile函数形参名调整

2018/12/24 v1.21.10</br>
-删除dxErr库，Windows SDK 8.0以上已内置错误码消息映射</br>
-清理无用文件

2018/12/21 v1.21.9</br>
-修复Win7部分项目在x86下编译失败的问题

2018/12/20 v1.21.8</br>
-添加Win7配置的项目和解决方案</br>
-项目21 添加鼠标点击物体后的响应

2018/12/15 v1.21.7</br>
-Geometry现支持多种形式的顶点(除VertexPosSize)</br>
-项目代码大幅度调整，为Geometry进行适配

2018/12/12 v1.21.6</br>
-修复Model使用Geometry时无法显示的异常

2018/12/11 v1.21.5</br>
-添加文档帮助用户解决项目无法编译、运行的问题</br>
-代码调整

2018/12/6 v1.21.4</br>
-对于不支持Direct2D显示的系统，将不显示文本</br>
-修改项目08标题问题</br>
-程序现在都生成到项目路径中

2018/12/4 v1.21.3</br>
-调整GameTimer</br>
-编译好的着色器文件后缀统一为.cso</br>
-调整部分Effects

2018/11/19 v1.21.2</br>
-修复项目17异常

2018/11/18 v1.21.1</br>
-调整D3DApp</br>
-修复项目24异常

2018/11/17 v1.21.0</br>
-**添加项目 Render To Texture**</br>
-调整RenderStates的初始化</br>
-调整初始化2D纹理时MipLevels的赋值</br>
-删除以10个为单位的解决方案</br>
-项目SDK版本更新至(17763)

2018/11/9 v1.20.4</br>
-添加AppVeyor，帮助检测项目编译情况(进行全属性配置编译)</br>
-添加用于检测的解决方案DirectX11 With Windows SDK.sln

2018/11/7 v1.20.3</br>
-修正第一/三人称视角程序窗口不在中心的问题</br>
-由于Windows SDK 8.1下DirectXMath.h的类构造函数为Trivial，需要人工初始化，如：XMFLOAT3(0.0f, 0.0f, 0.0f)。

2018/11/5 v1.20.2</br>
-将调用IUnknown::QueryInterface改成ComPtr调用As方法

2018/11/4 v1.20.1</br>
-修正Geometry类因多个重载支持无法编译通过的问题</br>
-修正项目16无法运行的问题</br>
-**提取了DXTK的键鼠类，并砍掉了DXTK库，现在每个项目可以独自编译了**

2018/11/2 v1.20.0</br>
-**添加项目 Dynamic Cube Mapping**</br>
-再次修正Geometry类圆柱显示问题</br>
-修正项目21-22中关于GameObject类忘记转置的问题

2018/10/29 v1.19.1</br>
-从.fx换成.hlsli后就不需要人工标记"不需要生成"了

2018/10/28 v1.19.0</br>
-**添加项目 Static Cube Mapping**</br>
-修正Geometry类问题

2018/10/26 v1.18.5</br>
-修正了项目12反射光照异常的问题

2018/10/25 v1.18.4</br>
-修正了GameObject类的一点小问题

2018/10/24 v1.18.3</br>
-顶点/索引缓冲区的Usage改为D3D11_USAGE_IMMUTABLE</br>
-顶点着色器变量名微调</br>
-Vertex.h微调</br>
-d3dUtil.h中的CreateDDSTexture2DArrayShaderResourceView更名为CreateDDSTexture2DArrayFromFile</br>
-考虑到不使用预编译头，所有项目头文件结构大幅调整</br>
-DXTK属性配置表微调</br>

2018/10/20 v1.18.2</br>
-将BasicFX和BasicObjectFX都更名为BasicEffect</br>
-将文件的.fx后缀改为.hlsli

2018/10/19 v1.18.1</br>
-优化着色器(减少汇编指令)

2018/10/17 v1.18.0</br>
-**添加项目Picking**</br>
-修正d3dUtil.h中HR宏的问题

2018/9/29 v1.17.1</br>
-添加d3dUtil.h</br>
-添加Camera关于ViewPort的方法</br>
-精简头文件</br>
-修改错误描述

2018/9/25 v1.17.0</br>
-**添加项目 Instancing and Frustum Culling**</br>
-删掉了到现在都没什么用的texTransform</br>
-调整注释

2018/9/18 v1.16.3</br>
-项目13, 14, 15, 16, 17, 19已更新为自己写的Effects框架

2018/9/17 v1.16.2</br>
-重构项目13，重新编写简易Effects框架</br>
-重新调整一些变量名和函数名，消灭了一些warning</br>
-解决了项目19字符集问题

2018/9/12 v1.16.1</br>
-修改ObjReader，解决镜像问题

2018/9/11 v1.16.0</br>
-**添加项目 Meshes**

2018/8/19 v1.15.0</br>
-**添加项目 Tree Billboard**</br>
-补回4倍多重采样的支持设置</br>
-调整代码细节

2018/8/12 v1.14.1</br>
-解决新项目x64编译无法通过的问题

2018/8/11 v1.14.0</br>
-**添加项目 Stream Output**

2018/8/9 v1.13.1</br>
-修正GameObject类的小错误

2018/8/8 v1.13.0</br>
-**添加项目 Geometry Shader Beginning**</br>
-小幅度修改键盘输入和着色器变量名标识问题

2018/8/8 v1.12.3</br>
-重新调整运行时编译/读取着色器的所有相关代码

2018/8/7 v1.12.2</br>
-修正项目11-13无法运行的问题

2018/8/6 v1.12.1</br>
-将所有项目的着色器代码都放到对应的hlsl文件中</br>
-细化Living Without FX11的绘制顺序

2018/8/5 v1.12.0</br>
-添加项目 Depth Test</br>
-编译二进制着色器文件的后缀名变成*.vso, *.pso, *.hso, *.gso, *.hso, *.cso</br>
-修正了Geometry类中圆柱体的模型问题，并添加了只包含圆柱侧面的方法</br>
-BasicManager类更名为BasicFX类</br>
-一些小修改

2018/7/29 v1.11.0</br>
-**添加项目 Living Without FX11**

2018/7/28 v1.10.0</br>
-**添加项目 Depth and Stenciling**</br>
-修正了光照、打包问题</br>
-修正了鼠标RELATIVE模式无法使用的问题

2018/7/24 v1.9.2</br>
-添加.gitignore

2018/7/23 v1.9.1</br>
-将着色器模型从4.0换成5.0

2018/7/21 v1.9.0</br>
-**添加项目 Blending**

2018/7/20 v1.8.1</br>
-修改了Material结构体构造函数的问题</br>
-每个解决方案将管理10章内容的项目，减小单个解决方案编译的项目数

2018/7/16 v1.8.0</br>
-**添加项目 Camera**</br>
-添加DirectXTK源码，删去旧有库，用户需要自己编译

2018/7/14 v1.7.2</br>
-修改了光照高亮(镜面反射)部分显示问题

2018/7/13 v1.7.1</br>
-HLSL和纹理分别使用单独的文件夹</br>
-消除x64平台下的一些警告

2018/7/12 v1.7.0</br>
-**添加项目 Texture Mapping**</br>
-原DX11 Without DirectX SDK 现在更名为 DirectX11 With Windows SDK

2018/6/21 v1.6.1</br>
-调整项目配置

2018/5/29 v1.6.0</br>
-**添加项目 Direct2D and Direct3D Interoperability**

2018/5/28 v1.5.0</br>
-**添加项目 Lighting**

2018/5/21 v1.4.1</br>
-小幅度问题修改

2018/5/14 v1.4.0</br>
-**添加项目 Mouse and Keyboard**</br>
-小幅度问题修改

2018/5/13 v1.3.0</br>
-**添加项目 Rendering a Cube**</br>
-**添加项目 Rendering a Triangle**

2018/5/12 v1.1.0</br>
-**添加项目 DirectX11 Initialization**
