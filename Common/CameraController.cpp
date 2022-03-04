#include "CameraController.h"
#include "d3dUtil.h"
#include <imgui.h>

using namespace DirectX;

void CameraController::ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
{
	float blendedValue;
	if (fabs(newValue) > fabs(oldValue))
		blendedValue = XMath::Lerp(newValue, oldValue, powf(0.6f, deltaTime * 60.0f));
	else
		blendedValue = XMath::Lerp(newValue, oldValue, powf(0.8f, deltaTime * 60.0f));
	oldValue = blendedValue;
	newValue = blendedValue;
}

void FirstPersonCameraController::Update(float deltaTime)
{
	ImGuiIO& io = ImGui::GetIO();

	float yaw = 0.0f, pitch = 0.0f, forward = 0.0f, strafe = 0.0f;
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{

		yaw += io.MouseDelta.x * m_MouseSensitivityX;
		pitch += io.MouseDelta.y * m_MouseSensitivityY;
	}

	forward = m_MoveSpeed * (
		(ImGui::IsKeyDown('W') ? deltaTime : 0.0f) +
		(ImGui::IsKeyDown('S') ? -deltaTime : 0.0f)
		);
	strafe = m_StrafeSpeed * (
		(ImGui::IsKeyDown('A') ? -deltaTime : 0.0f) +
		(ImGui::IsKeyDown('D') ? deltaTime : 0.0f)
		);

	if (m_Momentum)
	{
		ApplyMomentum(m_LastForward, forward, deltaTime);
		ApplyMomentum(m_LastStrafe, strafe, deltaTime);
	}

	m_pCamera->RotateY(yaw);
	m_pCamera->Pitch(pitch);

	m_pCamera->MoveForward(forward);
	m_pCamera->Strafe(strafe);
}

void FirstPersonCameraController::InitCamera(FirstPersonCamera* pCamera)
{
	m_pCamera = pCamera;
}

void FirstPersonCameraController::SlowMovement(bool enable)
{
	m_FineMovement = enable;
}

void FirstPersonCameraController::SlowRotation(bool enable)
{
	m_FineRotation = enable;
}

void FirstPersonCameraController::EnableMomentum(bool enable)
{
	m_Momentum = enable;
}

void FirstPersonCameraController::SetMouseSensitivity(float x, float y)
{
	m_MouseSensitivityX = x;
	m_MouseSensitivityY = y;
}

void FirstPersonCameraController::SetMoveSpeed(float speed)
{
	m_MoveSpeed = speed;
}

void FirstPersonCameraController::SetStrafeSpeed(float speed)
{
	m_StrafeSpeed = speed;
}
