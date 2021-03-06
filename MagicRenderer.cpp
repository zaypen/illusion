#include "MagicRenderer.h"

#include "Sensor.h"
#include "RenderDevice.h"

static const LPCWSTR FIRE_FILENAME_FORMAT = L"magic/fire%04d.tif";
static const int FIRE_FRAME_COUNT = 60;
static const D2D1_SIZE_U FIRE_FRAME_SIZE = D2D1::SizeU(160, 320);

static const LPCWSTR STAR_FILENAME_FORMAT = L"magic/star%04d.tif";
static const int STAR_FRAME_COUNT = 60;
static const D2D1_SIZE_U STAR_FRAME_SIZE = D2D1::SizeU(640, 480);

MagicRenderer::MagicRenderer() : m_nFireFrame(0), m_nStarFrame(0)
{
	m_pFireBitmaps = new ID2D1Bitmap* [FIRE_FRAME_COUNT]{nullptr};
	m_pStarBitmaps = new ID2D1Bitmap* [STAR_FRAME_COUNT]{nullptr};
}


MagicRenderer::~MagicRenderer()
{
}

void MagicRenderer::Initialize()
{
	for (auto i = 0; i < FIRE_FRAME_COUNT; ++i)
	{
		wchar_t buf[256];
		wsprintf(buf, FIRE_FILENAME_FORMAT, i + 1);
		m_pRenderDevice->LoadImageBitmap(buf, &m_pFireBitmaps[i]);
	}
	for (auto i = 0; i < STAR_FRAME_COUNT; ++i)
	{
		wchar_t buf[256];
		wsprintf(buf, STAR_FILENAME_FORMAT, i + 1);
		m_pRenderDevice->LoadImageBitmap(buf, &m_pStarBitmaps[i]);
	}
}

void MagicRenderer::Deinitialize()
{
	for (auto i = 0; i < FIRE_FRAME_COUNT; ++i)
	{
//		SafeRelease(m_pFireBitmaps[i]);
	}
}

void MagicRenderer::Render(RGBQUAD* pColor, IBody** ppBodies)
{
	const static int threshold = 15;
	static bool leftTracked[BODY_COUNT] = { false };
	static bool rightTracked[BODY_COUNT] = { false };
	static int leftLostTimes[BODY_COUNT] = { 0 };
	static int rightLostTimes[BODY_COUNT] = { 0 };
	for (int i = 0; i < BODY_COUNT; ++i)
	{
		IBody* pBody = ppBodies[i];
		if (pBody)
		{
			BOOLEAN bTracked = false;
			pBody->get_IsTracked(&bTracked);

			if (bTracked)
			{
				Joint joints[JointType_Count];
				D2D1_POINT_2F jointPoints[JointType_Count];
				HandState leftHandState = HandState_Unknown;
				HandState rightHandState = HandState_Unknown;

				pBody->get_HandLeftState(&leftHandState);
				pBody->get_HandRightState(&rightHandState);
				pBody->GetJoints(_countof(joints), joints);
				ColorSpacePoint leftHandPoint = { 0 }, rightHandPoint = { 0 };
				m_pSensor->CoordinateMapper()->MapCameraPointToColorSpace(joints[JointType_HandRight].Position, &rightHandPoint);
				m_pSensor->CoordinateMapper()->MapCameraPointToColorSpace(joints[JointType_HandLeft].Position, &leftHandPoint);

				if (rightHandState == HandState_Open)
				{
					rightTracked[i] = true;
				}
				else if (++rightLostTimes[i] > threshold)
				{
					rightTracked[i] = false;
					rightLostTimes[i] = 0;
				}

				if (leftHandState == HandState_Open)
				{
					leftTracked[i] = true;
				}
				else if (++leftLostTimes[i] > threshold)
				{
					leftTracked[i] = false;
					leftLostTimes[i] = 0;
				}

				if (rightTracked[i])
				{
					rightHandPoint.X += 10.f;
					rightHandPoint.Y += 10.f;
					float dx = rightHandPoint.X - leftHandPoint.X;
					float dy = rightHandPoint.Y - leftHandPoint.Y;
					float d = sqrt(dx * dx + dy * dy);
					float scale = min(400.f / d, 2);
					D2D1_RECT_F fireRect;
					if (rightTracked[i] && d <= 400.f)
					{
						fireRect = D2D1::RectF(
							rightHandPoint.X - FIRE_FRAME_SIZE.width / 2 * scale,
							rightHandPoint.Y - FIRE_FRAME_SIZE.height * scale,
							rightHandPoint.X + FIRE_FRAME_SIZE.width / 2 * scale,
							rightHandPoint.Y);
					}
					else
					{
						fireRect = D2D1::RectF(
							rightHandPoint.X - FIRE_FRAME_SIZE.width / 2,
							rightHandPoint.Y - FIRE_FRAME_SIZE.height,
							rightHandPoint.X + FIRE_FRAME_SIZE.width / 2,
							rightHandPoint.Y);
					}
					m_pRenderDevice->RenderTarget()->DrawBitmap(m_pFireBitmaps[m_nFireFrame++], fireRect);
					m_nFireFrame %= FIRE_FRAME_COUNT;
				}
				else if (leftTracked[i])
				{
					static float scale_velocity = 0.02f;
					static float scale = 1.f;
					leftHandPoint.Y -= STAR_FRAME_SIZE.width / 4;
					scale += scale_velocity;
					if (scale <= 1.f || scale >= 1.5f ) { scale_velocity = -scale_velocity; }
					D2D1_RECT_F starRect = D2D1::RectF(
						leftHandPoint.X - STAR_FRAME_SIZE.width / 2 * scale,
						leftHandPoint.Y - STAR_FRAME_SIZE.height / 2 * scale,
						leftHandPoint.X + STAR_FRAME_SIZE.width / 2 * scale,
						leftHandPoint.Y + STAR_FRAME_SIZE.height / 2 * scale);
					m_pRenderDevice->RenderTarget()->DrawBitmap(m_pStarBitmaps[m_nStarFrame++], starRect);
					m_nStarFrame %= STAR_FRAME_COUNT;
				}

			}
		}
	}
}


