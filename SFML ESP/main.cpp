#include <iostream>
#include <thread>
#include <Windows.h>
#include "memory.h"
#include <cmath>
#include <SFML/Graphics.hpp>

//REMEMBER TO PUT THE REQUIRED SFML DLLS IN THE SAME DIRECTORY AS THE EXE FILE

namespace offsets {

	uintptr_t entlist = 0x1BFDDD8;
	uintptr_t localPlayer = 0x1BF14A0;
	uintptr_t health = 0x34C;

	uintptr_t yaw =  0x1E3D5C4; //relative to base
	uintptr_t pitch = 0x1E3D5C0; //relative to base

	uintptr_t kills = 0x1C04208; //relative to base
	uintptr_t velocity = 0x1BEA850; //relative to base

	
	uintptr_t JumpFlag = 0x3F8;

	//Feet Pos From PlayerPawn Class
	uintptr_t X = 0xF58;
	uintptr_t Y = 0xF5C;
	uintptr_t Z = 0xF60;


	uintptr_t HeadX = 0x3E24;
	uintptr_t HeadY = 0x3E28;
	uintptr_t HeadZ = 0x3E2C;

	uintptr_t TeamNum = 0x1008;

};

void MakeLine(sf::RenderWindow& window, float x1, float y1, float x2, float y2) {

	sf::Vertex line[2];
	line[0].position = { x1, y1 };
	line[1].position = { x2, y2 };
	window.draw(line, 2, sf::PrimitiveType::Lines);
}

int main() {
	const float PI = 3.1415926;
	int test = 1;

	std::cout << "PID: " << VARS::processId << " BDDR: " << VARS::baseAddress << "\n";

	const int GameWidthResolution = 1920;
	const int GameHeightResolution = 1080;

	int WindowCenterW = GameWidthResolution / 2;
	int WindowCenterH = GameHeightResolution / 2;

	int EspOffsets[] = { -7, -18 };

	float PixelAngleProjection = (GameWidthResolution / 1360.0f) * 9;

	sf::RenderWindow window(sf::VideoMode({ GameWidthResolution, GameHeightResolution - 1 }), "////");
	window.setPosition(sf::Vector2i(0, 0));

	//make window transparent
	HWND hwnd = window.getNativeHandle();

	LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	LONG style = GetWindowLong(hwnd, GWL_STYLE);

	style &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);

	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	while (window.isOpen())
	{

		if (GetAsyncKeyState(VK_NUMPAD8) & 1) {
			EspOffsets[1]--;
		}
		else if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
			EspOffsets[1]++;
		}
		else if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
			EspOffsets[0]--;
		}
		else if (GetAsyncKeyState(VK_NUMPAD6) & 1) {
			EspOffsets[0]++;
		}

		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				window.close();
		}
		window.clear(sf::Color(0, 0, 0, 0));

		uintptr_t localplayer = VARS::memRead<uintptr_t>(VARS::baseAddress + offsets::localPlayer);
		int localTeam = VARS::memRead<int>(localplayer + offsets::TeamNum);


		for (int i = 0; i < 32; i++) {

			uintptr_t Entity = VARS::memRead<uintptr_t>(VARS::baseAddress + offsets::entlist + (i * 0x8));
			int eHealth = VARS::memRead<int>(Entity + offsets::health);
			int eTeam = VARS::memRead<int>(Entity + offsets::TeamNum);
			
			if (eHealth > 0 && eHealth <= 100 && eTeam != localTeam) {

				float Xpos = VARS::memRead<float>(Entity + offsets::HeadX);
				float Ypos = VARS::memRead<float>(Entity + offsets::HeadY);
				float Zpos = VARS::memRead<float>(Entity + offsets::HeadZ);

				float LocalXpos = VARS::memRead<float>(localplayer + offsets::HeadX);
				float LocalYpos = VARS::memRead<float>(localplayer + offsets::HeadY);
				float LocalZpos = VARS::memRead<float>(localplayer + offsets::HeadZ);

				if (Xpos == 0 && Ypos == 0) {
					continue;
				}

				float DistanceX = Xpos - LocalXpos;
				float DistanceY = Ypos - LocalYpos;
				float DistanceZ = Zpos - LocalZpos;

				float YawCorrect = (180 / PI) * (atan2(DistanceY,DistanceX));
				float PitchCorrect = (180 / PI) * -atan(DistanceZ / hypot(DistanceX, DistanceY));

				float LocalYaw = VARS::memRead<float>(VARS::baseAddress + offsets::yaw);
				float LocalPitch = VARS::memRead<float>(VARS::baseAddress + offsets::pitch);

				float YawDisplacement = YawCorrect - LocalYaw;// YawCorrect - LocalYaw
				
				if (YawDisplacement > 180) {
					YawDisplacement = -(360 - YawDisplacement);
				}
				else if (YawDisplacement < -180) {
					YawDisplacement = 360 + YawDisplacement;
				}

				float PitchDisplacement = PitchCorrect - LocalPitch;

				//Formula found on excel :) (it describes height of player in pixels and its relationship with distance)
				float HeightAdjustment = (40624 * pow(hypot(DistanceX, DistanceY), -1.014)) * (GameHeightResolution / 768);
				
				if (abs(YawDisplacement) < 25) {

					int CrosshairX = WindowCenterW + EspOffsets[0];
					int CrosshairY = WindowCenterH + EspOffsets[1];

					float pxAngle = PixelAngleProjection * YawDisplacement;
					float pxPitch = PixelAngleProjection * PitchDisplacement;

					MakeLine(window, CrosshairX - pxAngle, CrosshairY + pxPitch, CrosshairX - pxAngle, CrosshairY + pxPitch + HeightAdjustment);
					MakeLine(window, CrosshairX, CrosshairY, CrosshairX - pxAngle, CrosshairY + pxPitch);
				}
			}
		}
		//end Current Frame
		window.display();
	}

	CloseHandle(VARS::processHandle);
	Beep(1000, 100);
	return 0;
}
