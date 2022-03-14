# 更新记录

- 大版本号 固定
- 中版本号 项目添加
- 小版本号 项目调整(随中版本号添加归零)

2022/3/14 Ver1.34.0

- 添加项目Tile-Based Deferred Shading

2022/3/12 Ver1.33.3

- 修正项目36所在颜色空间，对ImGui源码有修改
- 项目36更名为Deferred Rendering
- 替换Sponza模型图片以减小打包体积
- 部分代码调整

2022/3/6 Ver1.33.2

- 调整项目36代码

2022/3/4 v1.33.0

- **添加项目 Deferred Shading Beginning**
- 添加Common文件夹存放公共部分代码，但目前仅限36章项目使用
- 添加stb、tiny_obj_loader到Common文件夹中

2022/3/1 v1.32.5

- 部分项目添加ImGui：7/10/15/16/17/20/23/30/31/32/33/34/35，默认开启
- 所有项目的分辨率从800x600调整为1280x720
- 调整摄像机速度
- 调整cmake

2021/11/9 v1.32.4
- 修改cmake，删除VS2015/2017的解决方案与项目，推荐使用其它VS版本的使用cmake构建项目
- 调整部分项目着色器名称以适配cmake

2020/12/8 v1.32.3
- 修复CreateTexture2DArrayFromFile读取问题
- 项目17着色器代码微调

2020/12/6 v1.32.2
- 修复新版VS的编译器出现的C2102问题
- 少量其余修改

2020/7/19 v1.32.1
- EffectHelper相关函数调整

2020/7/19 v1.32.0
- **添加项目 Particle System**
- 创建纹理数组的函数统一为`CreateTexture2DArrayFromFile`
- 调整特效的SetEyePos函数
- 为EffectHelper添加对流输出的支持

2020/7/16 v1.31.2
- 修正Transform的GetUpAxisXM和GetForwardAxisXM

2020/7/15 v1.31.1
- 调整Effects.h

2020/7/14 v1.31.0
- **添加项目 Displacement Mapping**
- 修改Geometry的Cylinder以支持该项目的曲面细分实例
- 调整项目属性配置

2020/7/11 v1.30.0
- **添加项目 Tessellation**

2020/7/6 v1.29.1
- 调整项目 SSAO

2020/7/4 v1.29.0
- **添加项目 SSAO**
- 修正Transform的RotateAround
- 整合项目以减少重复资源
- EffectHelper添加对cbuffer默认值的支持
- 修改项目31

2020/6/25 v1.28.3
- 修正Transform类，调整Transform和Camera的一些函数
- 调整项目09- 11关于`Basic_PS_3D`的HLSL代码

2020/6/6 v1.28.2
- 添加阴影映射的透明物体绘制支持
- 修复项目问题

2020/6/5 v1.28.1
- 删除ShadowRender并将阴影贴图功能整合到TextureRender中
- VS 2017安装器只有10.0.17763.0的最新SDK，故还是下降至该版本

2020/6/3 v1.28.0
- **添加项目 Shadow Mapping**
- Windows SDK版本提升至10.0.19041.0
- 摄像机移动速度提升100%

2020/5/28 v1.27.3
- 添加cmake初版支持

2020/5/27 v1.27.2
- 添加Transform类并适配到项目中

2020/2/14 v1.27.1
- 少量代码调整

2020/2/14 v1.27.0
- **添加项目 Blur and Sobel**

2020/2/7 v1.26.1
- 项目 OIT防止窗口拉伸出现问题
- 调整`TextureRender::Init`方法
- 调整`CreateWICTexutre2DArrayFromFile`方法

2020/2/6 v1.26.0
- **添加项目 OIT**
- 调整`TextureRender::Begin`方法
- 调整项目文件布局

2020/2/1 v1.25.1
- 修复Geometry中Terrain创建问题
- hlsl和hlsli编码也统一使用UTF- 8 NO BOM

2020/1/20 v1.25.0
- **添加项目 Waves**
- 调整图形调试设置对象名的函数
- 默认项目和Geometry索引类型调整为DWORD(即32位无符号整数)
- 部分Geometry取消center参数
- 调整SkyRender、TextureRedner，统一使用InitResource初始化
- 更新MIT许可证年份

2020/1/9 v1.24.20
- 调整项目配置(项目05变为06)

2019/9/26 v1.24.19
- 由于VS2019 16.3.0开始仅有C++17起能使用filesystem，在此删除了filesystem相关

2019/5/31 v1.24.18
- 源文件编码尝试替换为UTF- 8(NO BOM)

2019/5/22 v1.24.17
- 调整项目16

2019/5/17 v1.24.16
- 调整VS2019项目配置

2019/5/10 v1.24.15
- 调整SkyRender

2019/5/1 v1.24.14
- 添加圆锥几何体
- 修复圆柱体网格数据
- 调整教程07 08

2019/4/23 v1.24.13
- 调整接口，形参降为一般指针
- Geometry类改为在namespace Geometry中实现
- 调整注释

2019/4/14 v1.24.12
- 图形调试器为所有可见对象具名化

2019/4/13 v1.24.11
- 消除漏掉的Warning (#3)

2019/4/12 v1.24.10
- 消除剩余的Warning (#3)

2019/4/11 v1.24.9
- 修改.sln文件属性
- 消除大部分Warning C26495，部分类添加默认构造函数 (#3)
- 修改LightHelper类成员拼写，并调整Geometry类

2019/4/10 v1.24.8
- VS2015的Windows SDK版本从8.1上调成10.0.14393.0
- 修复一些项目的错误配置

2019/4/8 v1.24.7
- 消除Warning C26439, C28251和小部分C26495 (#3)

2019/4/4 v1.24.6
- 添加VS2019项目，重新编排项目和解决方案
- 调整一些立方体的旋转初速

2019/3/20 v1.24.5
- 代码风格调整(m, g变m_, g_)，注释对齐

2019/3/17 v1.24.4
- 修正项目07, 08异常

2019/3/13 v1.24.3
- 修正FOV不统一问题(后期统一成pi/3)

2019/3/11 v1.24.2
- 修正项目07异常

2019/3/5 v1.24.1
- 简化项目26和27，去掉大量无用的方法和成员

2019/2/19 v1.24.0
- **添加项目27 Bitonic Sort**
- Mouse和Keyboard类添加包含Windows.h

2019/2/16 v1.23.5
- 修正平面镜反射的光照

2019/2/12 v1.23.4
- 修正一些代码小错误
- 文件编码恢复

2019/2/11 v1.23.3
- 修正SkyRender中读取纹理时创建mipmap的问题
- d3dUtil.h添加缓冲区函数
- 修正Collision中立方体线框创建问题

2019/2/6 v1.23.2
- 源代码文件编码格式修改为UTF- 8 BOM(不需要考虑跨平台)
- HLSL相关文件编码格式修改为UTF- 8(fxc编译hlsl不支持UTF- 8 BOM)

2019/2/6 v1.23.1
- 常量缓冲区USAGE改为DYNAMIC

2019/1/31 v1.23.0
- **添加项目26 Compute Shader Beginning**
- 调整项目10 摄像机切换时的初始视角

2019/1/27 v1.22.3
- 调整注释
- 移除BasicEffect::SetWorldViewProjMatrix方法
- 更新教程无法编译、运行的方法

2019/1/14 v1.22.2
- 移除d3dUtil、EffectHelper对DXTrace的依赖
- 修改函数`CreateWICTextureCubeFromFile`为`CreateWICTexture2DCubeFromFile`
- 修复天空盒无法创建mipmaps的问题

2019/1/9 v1.22.1
- 修正第三人称摄像机的旋转
- 第三人称摄像机添加方法SetRotationX和SetRotationY
- 修正某些BasicEffect常量缓冲区标记更新索引不一致问题

2019/1/6 v1.22.0
- **添加项目25 Normal Mapping**
- 添加DXTrace
- 调整mbo格式以及物体材质
- 添加Modules文件夹，存放可复用的模块

2019/1/2 v1.21.12
- 所有的项目默认开启4倍多重采样
- 添加、更新著作权年份与内容

2018/12/31 v1.21.11
- 修复项目09- 14着色器入口点不匹配的问题
- CreateShaderFromFile函数形参名调整

2018/12/24 v1.21.10
- 删除dxErr库，Windows SDK 8.0以上已内置错误码消息映射
- 清理无用文件

2018/12/21 v1.21.9
- 修复Win7部分项目在x86下编译失败的问题

2018/12/20 v1.21.8
- 添加Win7配置的项目和解决方案
- 项目21 添加鼠标点击物体后的响应

2018/12/15 v1.21.7
- Geometry现支持多种形式的顶点(除VertexPosSize)
- 项目代码大幅度调整，为Geometry进行适配

2018/12/12 v1.21.6
- 修复Model使用Geometry时无法显示的异常

2018/12/11 v1.21.5
- 添加文档帮助用户解决项目无法编译、运行的问题
- 代码调整

2018/12/6 v1.21.4
- 对于不支持Direct2D显示的系统，将不显示文本
- 修改项目08标题问题
- 程序现在都生成到项目路径中

2018/12/4 v1.21.3
- 调整GameTimer
- 编译好的着色器文件后缀统一为.cso
- 调整部分Effects

2018/11/19 v1.21.2
- 修复项目17异常

2018/11/18 v1.21.1
- 调整D3DApp
- 修复项目24异常

2018/11/17 v1.21.0
- **添加项目 Render To Texture**
- 调整RenderStates的初始化
- 调整初始化2D纹理时MipLevels的赋值
- 删除以10个为单位的解决方案
- 项目SDK版本更新至(17763)

2018/11/9 v1.20.4
- 添加AppVeyor，帮助检测项目编译情况(进行全属性配置编译)
- 添加用于检测的解决方案DirectX11 With Windows SDK.sln

2018/11/7 v1.20.3
- 修正第一/三人称视角程序窗口不在中心的问题
- 由于Windows SDK 8.1下DirectXMath.h的类构造函数为Trivial，需要人工初始化，如：XMFLOAT3(0.0f, 0.0f, 0.0f)。

2018/11/5 v1.20.2
- 将调用IUnknown::QueryInterface改成ComPtr调用As方法

2018/11/4 v1.20.1
- 修正Geometry类因多个重载支持无法编译通过的问题
- 修正项目16无法运行的问题
- **提取了DXTK的键鼠类，并砍掉了DXTK库，现在每个项目可以独自编译了**

2018/11/2 v1.20.0
- **添加项目 Dynamic Cube Mapping**
- 再次修正Geometry类圆柱显示问题
- 修正项目21- 22中关于GameObject类忘记转置的问题

2018/10/29 v1.19.1
- 从.fx换成.hlsli后就不需要人工标记"不需要生成"了

2018/10/28 v1.19.0
- **添加项目 Static Cube Mapping**
- 修正Geometry类问题

2018/10/26 v1.18.5
- 修正了项目12反射光照异常的问题

2018/10/25 v1.18.4
- 修正了GameObject类的一点小问题

2018/10/24 v1.18.3
- 顶点/索引缓冲区的Usage改为D3D11_USAGE_IMMUTABLE
- 顶点着色器变量名微调
- Vertex.h微调
- d3dUtil.h中的CreateDDSTexture2DArrayShaderResourceView更名为CreateDDSTexture2DArrayFromFile
- 考虑到不使用预编译头，所有项目头文件结构大幅调整
- DXTK属性配置表微调

2018/10/20 v1.18.2
- 将BasicFX和BasicObjectFX都更名为BasicEffect
- 将文件的.fx后缀改为.hlsli

2018/10/19 v1.18.1
- 优化着色器(减少汇编指令)

2018/10/17 v1.18.0
- **添加项目Picking**
- 修正d3dUtil.h中HR宏的问题

2018/9/29 v1.17.1
- 添加d3dUtil.h
- 添加Camera关于ViewPort的方法
- 精简头文件
- 修改错误描述

2018/9/25 v1.17.0
- **添加项目 Instancing and Frustum Culling**
- 删掉了到现在都没什么用的texTransform
- 调整注释

2018/9/18 v1.16.3
- 项目13, 14, 15, 16, 17, 19已更新为自己写的Effects框架

2018/9/17 v1.16.2
- 重构项目13，重新编写简易Effects框架
- 重新调整一些变量名和函数名，消灭了一些warning
- 解决了项目19字符集问题

2018/9/12 v1.16.1
- 修改ObjReader，解决镜像问题

2018/9/11 v1.16.0
- **添加项目 Meshes**

2018/8/19 v1.15.0
- **添加项目 Tree Billboard**
- 补回4倍多重采样的支持设置
- 调整代码细节

2018/8/12 v1.14.1
- 解决新项目x64编译无法通过的问题

2018/8/11 v1.14.0
- **添加项目 Stream Output**

2018/8/9 v1.13.1
- 修正GameObject类的小错误

2018/8/8 v1.13.0
- **添加项目 Geometry Shader Beginning**
- 小幅度修改键盘输入和着色器变量名标识问题

2018/8/8 v1.12.3
- 重新调整运行时编译/读取着色器的所有相关代码

2018/8/7 v1.12.2
- 修正项目11- 13无法运行的问题

2018/8/6 v1.12.1
- 将所有项目的着色器代码都放到对应的hlsl文件中
- 细化Living Without FX11的绘制顺序

2018/8/5 v1.12.0
- 添加项目 Depth Test
- 编译二进制着色器文件的后缀名变成*.vso, *.pso, *.hso, *.gso, *.hso, *.cso
- 修正了Geometry类中圆柱体的模型问题，并添加了只包含圆柱侧面的方法
- BasicManager类更名为BasicFX类
- 一些小修改

2018/7/29 v1.11.0
- **添加项目 Living Without FX11**

2018/7/28 v1.10.0
- **添加项目 Depth and Stenciling**
- 修正了光照、打包问题
- 修正了鼠标RELATIVE模式无法使用的问题

2018/7/24 v1.9.2
- 添加.gitignore

2018/7/23 v1.9.1
- 将着色器模型从4.0换成5.0

2018/7/21 v1.9.0
- **添加项目 Blending**

2018/7/20 v1.8.1
- 修改了Material结构体构造函数的问题
- 每个解决方案将管理10章内容的项目，减小单个解决方案编译的项目数

2018/7/16 v1.8.0
- **添加项目 Camera**
- 添加DirectXTK源码，删去旧有库，用户需要自己编译

2018/7/14 v1.7.2
- 修改了光照高亮(镜面反射)部分显示问题

2018/7/13 v1.7.1
- HLSL和纹理分别使用单独的文件夹
- 消除x64平台下的一些警告

2018/7/12 v1.7.0
- **添加项目 Texture Mapping**
- 原DX11 Without DirectX SDK 现在更名为 DirectX11 With Windows SDK

2018/6/21 v1.6.1
- 调整项目配置

2018/5/29 v1.6.0
- **添加项目 Direct2D and Direct3D Interoperability**

2018/5/28 v1.5.0
- **添加项目 Lighting**

2018/5/21 v1.4.1
- 小幅度问题修改

2018/5/14 v1.4.0
- **添加项目 Mouse and Keyboard**
- 小幅度问题修改

2018/5/13 v1.3.0
- **添加项目 Rendering a Cube**
- **添加项目 Rendering a Triangle**

2018/5/12 v1.1.0
- **添加项目 DirectX11 Initialization**
