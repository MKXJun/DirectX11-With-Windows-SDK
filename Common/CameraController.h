//***************************************************************************************
// Camera.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// 提供第一人称(自由视角)的简易控制器，需要ImGui
// Provide 1st person(free view) camera controller. ImGui is required.
//***************************************************************************************

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "Camera.h"

class CameraController
{
public:
	CameraController() = default;
	CameraController& operator=(const CameraController&) = delete;
	virtual ~CameraController() {}
	virtual void Update(float deltaTime) = 0;

	// Helper function
	static void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);
};

class FirstPersonCameraController : public CameraController
{
public:

	~FirstPersonCameraController() override {};
	void Update(float deltaTime) override;

	void InitCamera(FirstPersonCamera* pCamera);
	
	void SlowMovement(bool enable);
	void SlowRotation(bool enable);
	void EnableMomentum(bool enable);

	void SetMouseSensitivity(float x, float y);
	void SetMoveSpeed(float speed);
	void SetStrafeSpeed(float speed);

	

private:
	FirstPersonCamera* m_pCamera = nullptr;

	float m_MoveSpeed = 5.0f;
	float m_StrafeSpeed = 5.0f;
	float m_MouseSensitivityX = 0.005f;
	float m_MouseSensitivityY = 0.005f;

	float m_CurrentYaw = 0.0f;
	float m_CurrentPitch = 0.0f;

	bool m_FineMovement = false;
	bool m_FineRotation = false;
	bool m_Momentum = true;

	float m_LastForward = 0.0f;
	float m_LastStrafe = 0.0f;
};



#endif
