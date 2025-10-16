# MiniGame-2D

작은 DirectX 2D 게임을 만들어보자.

## 구현 예정 항목
1. 렌더링 이벤트 루프
2. 렌더링
3. 입력 처리 (이동)
4. 완료

## 현재 구현 기능
1. 리소스 로딩
2. 2D 오브젝트 및 UI (간단 폰트 - ImGui) 렌더링
3. 이벤트 루프 처리
4. 입력 처리

## 개발 환경 및 스택
- Windows 10/11 x64
- Visual Studio 2022 (MSVC v143, C++17)
- Windows 10 SDK, DirectX 11
- Dear ImGui (Win32 + DX11 backends)
- CMake/Visual Studio 프로젝트 파일

## 빌드 (Windows)
1. Visual Studio 2022 Desktop development with C++ 워크로드와 Windows 10 SDK를 설치한다.
2. 리포지토리를 클론한 뒤 `external/imgui` 서브모듈(또는 복제본)을 준비한다.
3. `MiniGame-2D.sln`을 열어 원하는 구성/플랫폼(예: Debug x64)으로 빌드한다.
4. 빌드가 완료되면 `x64\Debug\MiniGame-2D.exe`를 실행한다.

## macOS
현재 미지원(향후 Metal 기반 구현을 추가할 예정).
