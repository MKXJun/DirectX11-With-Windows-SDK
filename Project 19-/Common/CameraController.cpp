#include "CameraController.h"
#include "XUtil.h"
#include <imgui.h>

using namespace DirectX;

void FirstPersonCameraController::Update(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();

    float yaw = 0.0f, pitch = 0.0f;
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {

        yaw += io.MouseDelta.x * m_MouseSensitivityX;
        pitch += io.MouseDelta.y * m_MouseSensitivityY;
    }

    int forward = (
        (ImGui::IsKeyDown(ImGuiKey_W) ? 1 : 0) +
        (ImGui::IsKeyDown(ImGuiKey_S) ? -1 : 0)
        );
    int strafe = (
        (ImGui::IsKeyDown(ImGuiKey_A) ? -1 : 0) +
        (ImGui::IsKeyDown(ImGuiKey_D) ? 1 : 0)
        );

    if (forward || strafe)
    {
        XMVECTOR dir = m_pCamera->GetLookAxisXM() * (float)forward + m_pCamera->GetRightAxisXM() * (float)strafe;
        XMStoreFloat3(&m_MoveDir, dir);
        m_MoveVelocity = m_MoveSpeed;
        m_DragTimer = m_TotalDragTimeToZero;
        m_VelocityDrag = m_MoveSpeed / m_DragTimer;
    }
    else
    {
        if (m_DragTimer > 0.0f)
        {
            m_DragTimer -= deltaTime;
            m_MoveVelocity -= m_VelocityDrag * deltaTime;
        }
        else
        {
            m_MoveVelocity = 0.0f;
        }
    }

    m_pCamera->RotateY(yaw);
    m_pCamera->Pitch(pitch);

    m_pCamera->Translate(m_MoveDir, m_MoveVelocity * deltaTime);
}

void FirstPersonCameraController::InitCamera(FirstPersonCamera* pCamera)
{
    m_pCamera = pCamera;
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
