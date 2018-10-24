#ifndef CAMERA_H
#define CAMERA_H

#include <d3d11_1.h>
#include <DirectXMath.h>

class Camera
{
public:
	Camera();
	virtual ~Camera() = 0;

	// 获取摄像机位置
	DirectX::XMVECTOR GetPositionXM() const;
	DirectX::XMFLOAT3 GetPosition() const;

	// 获取摄像机的坐标轴向量
	DirectX::XMVECTOR GetRightXM() const;
	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMVECTOR GetUpXM() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMVECTOR GetLookXM() const;
	DirectX::XMFLOAT3 GetLook() const;

	// 获取视锥体信息
	float GetNearWindowWidth() const;
	float GetNearWindowHeight() const;
	float GetFarWindowWidth() const;
	float GetFarWindowHeight() const;

	// 获取矩阵
	DirectX::XMMATRIX GetViewXM() const;
	DirectX::XMMATRIX GetProjXM() const;
	DirectX::XMMATRIX GetViewProjXM() const;

	// 获取视口
	D3D11_VIEWPORT GetViewPort() const;


	// 设置视锥体
	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);

	// 设置视口
	void SetViewPort(const D3D11_VIEWPORT& viewPort);
	void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

	// 更新观察矩阵
	virtual void UpdateViewMatrix() = 0;
protected:
	// 摄像机的观察空间坐标系对应在世界坐标系中的表示
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT3 mRight;
	DirectX::XMFLOAT3 mUp;
	DirectX::XMFLOAT3 mLook;
	
	// 视锥体属性
	float mNearZ;
	float mFarZ;
	float mAspect;
	float mFovY;
	float mNearWindowHeight;
	float mFarWindowHeight;

	// 观察矩阵和透视投影矩阵
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	// 当前视口
	D3D11_VIEWPORT mViewPort;

};

class FirstPersonCamera : public Camera
{
public:
	FirstPersonCamera();
	~FirstPersonCamera() override;

	// 设置摄像机位置
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	// 设置摄像机的朝向
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target,const DirectX::XMFLOAT3& up);
	void LookTo(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR to, DirectX::FXMVECTOR up);
	void LookTo(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& up);
	// 平移
	void Strafe(float d);
	// 直行(平面移动)
	void Walk(float d);
	// 前进(朝前向移动)
	void MoveForward(float d);
	// 上下观察
	void Pitch(float rad);
	// 左右观察
	void RotateY(float rad);


	// 更新观察矩阵
	void UpdateViewMatrix() override;
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() override;

	// 获取当前跟踪物体的位置
	DirectX::XMFLOAT3 GetTargetPosition() const;
	// 获取与物体的距离
	float GetDistance() const;
	// 获取绕X轴的旋转方向
	float GetRotationX() const;
	// 获取绕Y轴的旋转方向
	float GetRotationY() const;
	// 绕物体垂直旋转
	void RotateX(float rad);
	// 绕物体水平旋转
	void RotateY(float rad);
	// 拉近物体
	void Approach(float dist);
	// 设置并绑定待跟踪物体的位置
	void SetTarget(const DirectX::XMFLOAT3& target);
	// 设置初始距离
	void SetDistance(float dist);
	// 设置最小最大允许距离
	void SetDistanceMinMax(float minDist, float maxDist);
	// 更新观察矩阵
	void UpdateViewMatrix() override;

private:
	DirectX::XMFLOAT3 mTarget;
	float mDistance;
	// 最小允许距离，最大允许距离
	float mMinDist, mMaxDist;
	// 以世界坐标系为基准，当前的旋转角度
	float mTheta;
	float mPhi;
};


#endif