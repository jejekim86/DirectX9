// Direct3D_project.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")


#include "framework.h"
#include "D3DProject.h"
#include <d3d9.h> // 기본 Directx9
#include <d3dx9.h> // 확장 Directx9
#include "DIB.h"


#define MAX_LOADSTRING 100
#define Deg2Rad 0.017453293f
D3DXMATRIXA16 g_matEarth, g_matMoon, g_matSun; // 전역 변수.

LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // 정점 버퍼. 전역변수.
struct CUSTOMVERTEX
{
	float x, y, z; // 정점의 좌표.
	D3DXVECTOR3 normal; // 법선 벡터.
	D3DXVECTOR2 tex;
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )


LPDIRECT3DINDEXBUFFER9 g_pIB = NULL; // 인덱스 버퍼. 전역 변수.
struct INDEX
{
	DWORD _0, _1, _2;
};

//struct D3DVIEWPORT9
//{
//    DWORD x, y, width, height; // 픽셀 단위 사용.
//    float MinZ; // 일반적으로 0.0f을 사용.
//    float MaxZ; // 일반적으로 0.1f을 사용.
//};


HRESULT InitD3D(HWND);
HRESULT InitVertexBuffer();
HRESULT InitIndexBuffer();
void Cleanup();
void Render();
void Update();
void SetupMatrices();
void DrawMesh(const D3DXMATRIXA16& matrix);

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	LoadStringW(hInstance, IDC_D3DPROJECT, szWindowClass, MAX_LOADSTRING); // szWindowClass를 로드 할려고
	// 핵심 "szWindowClass"
	WNDCLASSEXW wcex = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		0,
		0,
		NULL,
		szWindowClass,
		NULL
	};
	RegisterClassExW(&wcex); // szWindowClass를 등록

	HWND hWnd = CreateWindowW(szWindowClass, L"D", WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) return FALSE;

	if (SUCCEEDED(InitD3D(hWnd)))
	{
		if (SUCCEEDED(InitVertexBuffer()))
		{
			if (SUCCEEDED(InitIndexBuffer()))
			{
				ShowWindow(hWnd, nCmdShow);
				UpdateWindow(hWnd);


				MSG msg;
				ZeroMemory(&msg, sizeof(msg));
				while (WM_QUIT != msg.message)
				{
					if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					else
					{
						Update();
						Render();
					}
				}
			}
			Cleanup();
		}
	}
	return 0;
} // wWinMain()

LPDIRECT3D9 g_pD3D = NULL; // D3D Device를 생성할 D3D 객체 변수. 전역 변수.
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
LPDIRECT3DTEXTURE9 g_earthTexture = NULL;
LPDIRECT3DTEXTURE9 g_sunTexture = NULL;
LPDIRECT3DTEXTURE9 g_moonTexture = NULL;

int g_vertexCount, g_indexCount; // 전역 변수.
int g_cx = 3, g_cz = 3; // 가로, 세로 정점의 수.

HRESULT InitD3D(HWND hWnd)
{

	// Device를 생성하기 위한 D3D객체.
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;
	// Device 생성을 위한 구조체.
	// Present는 백 버퍼의 내용을 스크린에 보여주는 작업을 한다.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp)); // 메모리의 쓰레기 값을 반드시 지원야 한다.
	d3dpp.Windowed = TRUE; // 창 모드로 생성.
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // 가장 효율적인 SWAP 효과. 백 버퍼의 내용을 프론트 버퍼로 Swap하는 방식.
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // 런타임에, 현재 디스플레이 모드에 맞춰 백 버퍼를 생성.
	d3dpp.EnableAutoDepthStencil = TRUE; // D3D에서 프로그램의 깊이 버퍼(Z-Buffer)를 관리하게 한다.

	d3dpp.AutoDepthStencilFormat = D3DFMT_D16; // 16비트의 깊이 버퍼를 사용.

	DWORD level;
	for (auto type = (int)D3DMULTISAMPLE_16_SAMPLES; 0 < type; type--)
	{
		if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_D16, FALSE, (D3DMULTISAMPLE_TYPE)type, &level)))
		{
			d3dpp.MultiSampleQuality = level - 1;
			d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)type;
			break;
		}
	}
	// Device 생성.
	// D3DADAPTER_DEFAULT : 기본 그래픽카드를 사용.
	// D3DDEVTYPE_HAL : 하드웨어 가속장치를 지원한다(그래픽카드 제조사에서 DirectX가 세부적인 부분을 제어할 필요가 없도록 지원하는 기능).
	// D3DCREATE_SOFTWARE_VERTEXPROCESSING : 정점(Vertex) 쉐이더 처리를 소프트웨어에서 한다(하드웨어에서 할 경우 더욱 높은 성능을 발휘:D3DCREATE_HARDWARE_VERTEXPROCESSING)
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	// TODO: 여기에서 Device 상태 정보 처리를 처리를 한다.
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // 컬링 모들를 켠다.
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // 광원 기능을 끈다.
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE); // Z(깊이) 버퍼 기능을 켠다.
	g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (0 < d3dpp.MultiSampleType));

	D3DMATERIAL9 mtrl;
	ZeroMemory(&mtrl, sizeof(mtrl));
	mtrl.Diffuse = mtrl.Ambient = { 1, 1, 1, 1 };
	g_pd3dDevice->SetMaterial(&mtrl);
	D3DXCreateTextureFromFile(g_pd3dDevice, L"earth.png", &g_earthTexture);
	D3DXCreateTextureFromFile(g_pd3dDevice, L"moon.jpg", &g_moonTexture);
	D3DXCreateTextureFromFile(g_pd3dDevice, L"sun.png", &g_sunTexture);

	return S_OK;
}


void Cleanup()
{
	// 반드시 생성의 역순으로 해제.
	if (NULL != g_pd3dDevice) g_pd3dDevice->Release();
	if (NULL != g_pD3D) g_pD3D->Release();
	if (NULL != g_pVB) g_pVB->Release();
	if (NULL != g_pIB) g_pIB->Release();
	if (NULL != g_earthTexture) g_earthTexture->Release();
	if (NULL != g_moonTexture) g_moonTexture->Release();
	if (NULL != g_sunTexture) g_sunTexture->Release();
}


HRESULT InitVertexBuffer()
{
	CUSTOMVERTEX vertices[24];
	for (int i = 0; 2 > i; i++) // 위, 아랫면 vertex buffer 설정.
	{
		int index = i * 4;
		vertices[index] = { -0.5f, 0.5f - i, 0.5f, D3DXVECTOR3(-0.5f, 0.5f - i, 0.5f), D3DXVECTOR2(0.25f, i) };
		vertices[index + 1] = { -0.5f, 0.5f - i, -0.5f, D3DXVECTOR3(-0.5f, 0.5f - i, -0.5f), D3DXVECTOR2(0.25f, i * 0.333333f + 0.333333f) };
		vertices[index + 2] = { 0.5f, 0.5f - i, -0.5f, D3DXVECTOR3(0.5f, 0.5f - i, -0.5f), D3DXVECTOR2(0.5f, i * 0.333333f + 0.333333f) };
		vertices[index + 3] = { 0.5f, 0.5f - i, 0.5f, D3DXVECTOR3(0.5f, 0.5f - i, 0.5f), D3DXVECTOR2(0.5f, i) };
	}
	for (int i = 0; 4 > i; i++)
	{
		int index = (i + 2) * 4;
		vertices[index] = vertices[Repeat(i, 3)]; // left, top
		vertices[index].tex = D3DXVECTOR2(i * 0.25f, 0.333333f);
		vertices[index + 1] = vertices[Repeat(i, 3) + 4]; // left, bottom
		vertices[index + 1].tex = D3DXVECTOR2(i * 0.25f, 0.666666f);
		vertices[index + 2] = vertices[Repeat(i + 1, 3) + 4]; // right, bottom
		vertices[index + 2].tex = D3DXVECTOR2((i + 1) * 0.25f, 0.666666f);
		vertices[index + 3] = vertices[Repeat(i + 1, 3)]; // right, top
		vertices[index + 3].tex = D3DXVECTOR2((i + 1) * 0.25f, 0.333333f);
	}


	// 정점 버퍼 생성.
	// 정점을 보관할 메모리 공간을 할당.
	// FVF를 지정하여 보관할 데이터 형식을 지정.
	// D3DPOOL_DEFAULT:리소스가 가장 적합한 메모리에 놓여진다.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(sizeof(vertices), 0, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &g_pVB, NULL)))
	{
		return E_FAIL;
	}
	// 정점 버퍼를 지정한 값으로 채운다.
	// 외부에서 접근하지 못하게 메모리를 Lock() 하고 사용이 끝난 후 Unlock()을 한다.
	LPVOID pVertices;
	if (FAILED(g_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) return E_FAIL;
	memcpy(pVertices, vertices, sizeof(vertices));
	g_pVB->Unlock();
	return S_OK;
}

void Render()
{
	if (NULL == g_pd3dDevice) return;
	// pRects(두 번째 인자)가 NULL이라면 Count(첫 번째 인자)는 0.
	// D3DCLEAR_TARGET:surface를 대상으로 한다.
	// D3DCLEAR_ZBUFFER:Z(깊이) 버퍼를 대상으로 한다.
	// 버퍼를 파란색[RGB(0, 0, 255)]으로 지운다.
	// Z(깊이) 버퍼를 1로 지운다. (0 ~ 1 사이의 값을 사용)
	// 스텐실(stencil) 버퍼를 0으로 지운다. (0 ~ 2^n-1 사이의 값을 사용)
	// 다음과 같은 경우 IDirect3DDevice9 :: Clear() 함수가 실패.
	// - 깊이 버퍼가 연결되지 않은 렌더링 대상의 깊이 버퍼 또는 스텐실 버퍼를 지운다.
	// - 깊이 버퍼에 스텐실 데이터가 포함되지 않은 경우 스텐실 버퍼를 지운다.
	if (SUCCEEDED(g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1, 0)))
	{
		SetupMatrices();

		if (SUCCEEDED(g_pd3dDevice->BeginScene()))
		{
			g_pd3dDevice->SetTexture(0, g_earthTexture);
			DrawMesh(g_matEarth);


			g_pd3dDevice->SetTexture(0, g_moonTexture);
			DrawMesh(g_matMoon);

			g_pd3dDevice->SetTexture(0, g_sunTexture);
			DrawMesh(g_matSun);

			g_pd3dDevice->EndScene();
		}
		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	}
}
HRESULT InitIndexBuffer()
{
	INDEX indices[12];
	for (int i = 0; 6 > i; i++)
	{
		int index = i * 2;
		DWORD value = i * 4;
		indices[index] = { value, value + 3, value + 2 };
		indices[index + 1] = { value , value + 2, value + 1 };
	}
	// 아랫면의 두르기 방향이 달라, 개별적 처리.
	indices[2] = { 5, 6, 7 };
	indices[3] = { 5, 7, 4 };

	// 인덱스 버퍼 생성.
	// D3DFMT_INDEX16:인덱스의 단위가 16비트. WORD(unsigned short):2byte(16bit).
	// D3DFMT_INDEX32:인덱스의 단위가 32비트. DWORD(unsigned long):4byte(32bit).
	if (FAILED(g_pd3dDevice->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL))) return E_FAIL;
	LPVOID pIndices;
	if (FAILED(g_pIB->Lock(0, sizeof(indices), (void**)&pIndices, 0))) return E_FAIL;
	memcpy(pIndices, indices, sizeof(indices));
	g_pIB->Unlock();
	return S_OK;
}



void SetupMatrices()
{
	// 월드 스페이스.
	D3DXMATRIXA16 matWorld; // 월드 행렬.
	// D3DXMatrixTranslation
	// D3DXMatrixRotationX,Y,Z
	// D3DXMatrixScaling
	// 등의 작업을 여기서 수행.
	D3DXMatrixIdentity(&matWorld); // 단위행렬로 변경.
	// 좌표를 (0, 0, 0) 변경.
	// D3DXMatrixTranslation(& matWorld, 0, 0, 0);
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // 월드 스페이스로 변환.

	D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f); // 월드 좌표의 카메라의 위치.
	D3DXVECTOR3 vLookAtPt(0.0f, 0.0f, 0.0f); // 월드 좌표의 카메라가 바라보는 위치.
	D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f); // 월드 좌표의 하늘 방향을 알기 위한 up vector.
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAtPt, &vUpVector); // 뷰 변환 행렬 계산.
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView); // 뷰 스페이스로 변환.

	D3DXMATRIXA16 matProj;
	// 투영 변환 행렬 계산.
	// FOV:시야각. 45도(D3DX_PI/4 == 45 * Deg2Rad).
	// 1.77f:종횡비. 1.0f(1:1), 1.33f(4:3), 1.77f/1.78f(16:9), 2.0f(18:9).......
	// 시야 거리는 1.0f(near)에서 100.0f(far)까지.
	D3DXMatrixPerspectiveFovLH(&matProj, 45 * Deg2Rad, 1.77f, 1.0f, 100.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj); // 투영 변환.
}




void Update()
{
	auto angle = GetTickCount64() * 0.001f;
	D3DXMATRIXA16 matSunTr, matSunRo;
	D3DXMatrixRotationY(&matSunRo, angle * 0.5f);
	D3DXMatrixTranslation(&matSunTr, 0, 0, 0);
	g_matSun = matSunTr;

	// 지구 행렬 설정
	D3DXMATRIXA16 matEarthRo, matEarthTr, matEarthSc;
	D3DXMatrixRotationY(&matEarthRo, angle * 0.5f); // 지구의 자전
	D3DXMatrixTranslation(&matEarthTr, 3, 0, 0); // 태양 주위를 공전
	D3DXMatrixScaling(&matEarthSc, 0.4f, 0.4f, 0.4f); // 크기 변경
	

	// 달 행렬 설정
	D3DXMATRIXA16 matMoonTr, matMoonRo, matMoonSc;
	D3DXMatrixRotationY(&matMoonRo, angle * 0.5f); // 달의 자전
	D3DXMatrixTranslation(&matMoonTr, 5, 0, 0); // 지구 주위를 공전
	D3DXMatrixScaling(&matMoonSc, 0.27f, 0.27f, 0.27f); // 달의 크기

	g_matEarth = matEarthSc * matEarthRo * matEarthTr * matSunRo; // 태양을 중심으로 하여 공전과 자전을 적용
	g_matMoon = matMoonSc * matMoonRo * matMoonTr * g_matEarth; // 부모의 정보 추가

}



void DrawMesh(const D3DXMATRIXA16& matrix)
{
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matrix);
	g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOM);
	g_pd3dDevice->SetIndices(g_pIB);
	g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		/*  if (VK_NUMPAD2 == wParam)
		  {
		  }*/
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 인덱스 버퍼까지 하고 나중에 안티 앨리어싱 적용 미적용 비교 해보기