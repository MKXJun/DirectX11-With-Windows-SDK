#include "Camera.h"
using namespace DirectX;

Camera::Camera()
{
}

Camera::~Camera()
{
}

DirectX::XMVECTOR Camera::GetPositionXM() const
{
	return XMLoadFloat3(&mPosition);
}

DirectX::XMFLOAT3 Camera::GetPosition() const
{
	return mPosition;
}

DirectX::XMVECTOR Camera::GetRightXM() const
{
	return XMLoadFloat3(&mRight);
}

DirectX::XMFLOAT3 Camera::GetRight() const
{
	return mRight;
}

DirectX::XMVECTOR Camera::GetUpXM() const
{
	return XMLoadFloat3(&mUp);
}

DirectX::XMFLOAT3 Camera::GetUp() const
{
	return mUp;
}

DirectX::XMVECTOR Camera::GetLookXM() const
{
	return XMLoadFloat3(&mLook);
}

DirectX::XMFLOAT3 Camera::GetLook() const
{
	return mLook;
}

float Camera::GetNearWindowWidth() const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return mFarWindowHeight;
}

DirectX::XMMATRIX Camera::GetViewXM() const
{
	return XMLoadFloat4x4(&mView);
}

DirectX::XMMATRIX Camera::GetProjXM() const
{
	return XMLoadFloat4x4(&mProj);
}

DirectX::XMMATRIX Camera::GetViewProjXM() const
{
	return XMLoadFloat4x4(&mView) * XMLoadFloat4x4(&mProj);
}

D3D11_VIEWPORT Camera::GetViewPort() const
{
	return mViewPort;
}

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = nearZ;
	mFarZ = farZ;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);

	XMStoreFloat4x4(&mProj, XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ));
}

void Camera::SetViewPort(const D3D11_VIEWPORT & viewPort)
{
	mViewPort = viewPort;
}

void Camera::SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
	mViewPort.TopLeftX = topLeftX;
	mViewPort.TopLeftY = topLeftY;
	mViewPort.Width = width;
	mViewPort.Height = height;
	mViewPort.MinDepth = minDepth;
	mViewPort.MaxDepth = maxDepth;
}


// ********************
// 第一人称/自由视角摄像机
//

FirstPersonCamera::FirstPersonCamera()
	: Camera()
{
}

FirstPersonCamera::~FirstPersonCamera()
{
}

void FirstPersonCamera::SetPosition(float x, float y, float z)
{
	SetPosition(XMFLOAT3(x, y, z));
}

void FirstPersonCamera::SetPosition(const DirectX::XMFLOAT3 & v)
{
	mPosition = v;
}

void FirstPersonCamera::LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up)
{
	LookTo(pos, target - pos, up);
}

void FirstPersonCamera::LookAt(const DirectX::XMFLOAT3 & pos, const DirectX::XMFLOAT3 & target,const DirectX::XMFLOAT3 & up)
{
	LookAt(XMLoadFloat3(&pos), XMLoadFloat3(&target), XMLoadFloat3(&up));
}

void FirstPersonCamera::LookTo(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR to, DirectX::FXMVECTOR up)
{
	XMVECTOR L = XMVector3Normalize(to);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(up, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
}

void FirstPersonCamera::LookTo(const DirectX::XMFLOAT3 & pos, const DirectX::XMFLOAT3 & to, const DirectX::XMFLOAT3 & up)
{
	LookTo(XMLoadFloat3(&pos), XMLoadFloat3(&to), XMLoadFloat3(&up));
}

void FirstPersonCamera::Strafe(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&mPosition);
	XMVECTOR Right = XMLoadFloat3(&mRight);
	XMVECTOR Dist = XMVectorReplicate(d);
	// DestPos = Dist * Right + SrcPos
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(Dist, Right, Pos));
}

void FirstPersonCamera::Walk(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&mPosition);
	XMVECTOR Right = XMLoadFloat3(&mRight);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Front = XMVector3Normalize(XMVector3Cross(Right, Up));
	XMVECTOR Dist = XMVectorReplicate(d);
	// DestPos = Dist * Front + SrcPos
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(Dist, Front, Pos));
}

void FirstPersonCamera::MoveForward(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&mPosition);
	XMVECTOR Look = XMLoadFloat3(&mLook);
	XMVECTOR Dist = XMVectorReplicate(d);
	// DestPos = Dist * Look + SrcPos
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(Dist, Look, Pos));
}

void FirstPersonCamera::Pitch(float rad)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), rad);
	XMVECTOR Up = XMVector3TransformNormal(XMLoadFloat3(&mUp), R);
	XMVECTOR Look = XMVector3TransformNormal(XMLoadFloat3(&mLook), R);
	float cosPhi = XMVectorGetY(Look);
	// 将上下视野角度Phi限制在[2pi/9, 7pi/9]，
	// 即余弦值[-cos(2pi/9), cos(2pi/9)]之间
	if (fabs(cosPhi) > cosf(XM_2PI / 9))
		return;
	
	XMStoreFloat3(&mUp, Up);
	XMStoreFloat3(&mLook, Look);
}

void FirstPersonCamera::RotateY(float rad)
{
	XMMATRIX R = XMMatrixRotationY(rad);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
}

void FirstPersonCamera::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&mRight);
	XMVECTOR U = XMLoadFloat3(&mUp);
	XMVECTOR L = XMLoadFloat3(&mLook);
	XMVECTOR P = XMLoadFloat3(&mPosition);

	// 保持摄像机的轴互为正交，且长度都为1
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L已经正交化，需要计算对应叉乘得到R
	R = XMVector3Cross(U, L);

	// 填充观察矩阵
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLook, L);

	mView = {
		mRight.x, mUp.x, mLook.x, 0.0f,
		mRight.y, mUp.y, mLook.y, 0.0f,
		mRight.z, mUp.z, mLook.z, 0.0f,
		x, y, z, 1.0f
	};
}

// ********************
// 第三人称摄像机
//

ThirdPersonCamera::ThirdPersonCamera()
	: Camera(), mTheta(), mPhi(), mDistance(), mTarget()
{
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

DirectX::XMFLOAT3 ThirdPersonCamera::GetTargetPosition() const
{
	return mTarget;
}

float ThirdPersonCamera::GetDistance() const
{
	return mDistance;
}

float ThirdPersonCamera::GetRotationX() const
{
	return mPhi;
}

float ThirdPersonCamera::GetRotationY() const
{
	return mTheta;
}

void ThirdPersonCamera::RotateX(float rad)
{
	mPhi -= rad;
	// 将上下视野角度Phi限制在[pi/6, pi/2]，
	// 即余弦值[0, cos(pi/6)]之间
	if (mPhi < XM_PI / 6)
		mPhi = XM_PI / 6;
	else if (mPhi > XM_PIDIV2)
		mPhi = XM_PIDIV2;
}

void ThirdPersonCamera::RotateY(float rad)
{
	mTheta = XMScalarModAngle(mTheta - rad);
}

void ThirdPersonCamera::Approach(float dist)
{
	mDistance += dist;
	// 限制距离在[mMinDist, mMaxDist]之间
	if (mDistance < mMinDist)
		mDistance = mMinDist;
	else if (mDistance > mMaxDist)
		mDistance = mMaxDist;
}

void ThirdPersonCamera::SetTarget(const DirectX::XMFLOAT3 & target)
{
	mTarget = target;
}

void ThirdPersonCamera::SetDistance(float dist)
{
	mDistance = dist;
}

void ThirdPersonCamera::SetDistanceMinMax(float minDist, float maxDist)
{
	mMinDist = minDist;
	mMaxDist = maxDist;
}

void ThirdPersonCamera::UpdateViewMatrix()
{
	// 球面坐标系
	float x = mTarget.x - mDistance * sinf(mPhi) * cosf(mTheta);
	float z = mTarget.z - mDistance * sinf(mPhi) * sinf(mTheta);
	float y = mTarget.y + mDistance * cosf(mPhi);
	mPosition = { x, y, z };
	XMVECTOR P = XMLoadFloat3(&mPosition);
	XMVECTOR L = XMVector3Normalize(XMLoadFloat3(&mTarget) - P);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), L));
	XMVECTOR U = XMVector3Cross(L, R);
	
	// 更新向量
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLook, L);

	mView = {
		mRight.x, mUp.x, mLook.x, 0.0f,
		mRight.y, mUp.y, mLook.y, 0.0f,
		mRight.z, mUp.z, mLook.z, 0.0f,
		-XMVectorGetX(XMVector3Dot(P, R)), -XMVectorGetX(XMVector3Dot(P, U)), -XMVectorGetX(XMVector3Dot(P, L)), 1.0f
	};
}

