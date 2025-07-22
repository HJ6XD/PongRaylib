/*
	Ejemplo de pong en red con raylib y Windows Sockets
	Debido a que raylib colisiona con funciones de Windows.h 
	se debe incluir Winsock2.h en Network.cpp, NO en main.cpp ni en Network.h
	Ver https://github.com/raysan5/raylib/issues/1217
*/
#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include <iostream>
#include <sstream>
#define LOG(x) std::cout << x << std::endl
#include "Network.h"	
#define PORT 7777
#define BUFFLEN 256 //Tamaño maximo de mensaje enviado por la red

enum EAPPState
{
	SERVER, CLIENT, MENU
} AppState;

void CreateServer();
void CreateClient();
void UpdateMenu();
void UpdateServer();
void UpdateClient();
void DrawMenu();
void DrawServer();
void DrawClient();
void DisplayMessage(bool bShow, const char* msg = "");
char* screenMsg = nullptr;

//TODO sustituir con un singleton
Network *network;
SocketHandle socket = nullptr;
SocketHandle clientSocket = nullptr; 

//parámetros de los jugadores
int playerWidth = 20;
int playerHeight; 
float playerSpeed = 500;
Vector2 player1Pos, player1Vel,  player2Pos, player2Vel, ballPos, ballVel;
//velocidad deseada, se establece con los inputs
Vector2 player1DesVel, player2DesVel;

int player1Score = 0, player2Score = 0;
std::string strp1s = "0", strp2s = "0";

int main ()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(800, 600, "Hello Raylib");
	SearchAndSetResourceDir("resources");

	playerWidth = 20;
	playerHeight = GetScreenHeight() / 5;
	network = new Network();
	if (!network->init()) {
		LOG("Error al inicializar networking");
		return -1;
	}
	AppState = EAPPState::MENU;

	ballVel = { 50,50 };

	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		switch (AppState)
		{
		case SERVER:
			UpdateServer();
			break;
		case CLIENT:
			UpdateClient();
			break;
		case MENU:
			UpdateMenu();
			break;
		default:
			break;
		}

		BeginDrawing();
		switch (AppState)
		{
		case SERVER:
			DrawServer();
			break;
		case CLIENT:
			DrawClient();
			break;
		case MENU:
			DrawMenu();
			break;
		default:
			break;
		}
		EndDrawing();
	}
	if(network)
	{
		network->shutdown();
		delete network;
		network = nullptr;
	}
	CloseWindow();
	return 0;
}

void CreateServer()
{
	LOG("Creando servidor");
	socket = network->createSocket();
	network->setNonBlocking(socket);
	if (socket==nullptr)
	{
		LOG("Error al crear socket");
		return;
	}
	if (!network->bindServer(socket, PORT))
	{
		LOG("Error al enlazar socket al puerto " << PORT);
		return;
	}
	AppState = EAPPState::SERVER;
	LOG("Servidor creado y escuchando en el puerto " << PORT);

	//El servidor es quien tiene la autoridad y establece las posiciones de los jugadores y la bola
	player1Pos = { 20, (float)GetScreenHeight() / 2 - playerHeight/2 };
	player1Vel = { 0, 0 };
	player2Pos = { (float)GetScreenWidth() - 40, (float)GetScreenHeight() / 2 - playerHeight / 2 };
	player2Vel = { 0, 0 };
	ballPos = { (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };

	LOG("sizeof sockaddr_in: " << network->sizeOfsockaddr_in());
}

void UpdateServer()
{
	//primero recibemos los datos del cliente
	char buffer[BUFFLEN];

	AddressHandle remoteAddress = malloc(network->sizeOfsockaddr_in());

	//debemos procesar todos los mensajes recibidos para evitar lag
	while (network->receiveFrom(socket, buffer, BUFFLEN, remoteAddress))
	{
		//LOG("Recibido: " << buffer);
		LOG( (remoteAddress == nullptr ? "null":"si existe" ));
		//deserializar los datos del cliente que vienen en formato "P2,x,y"
		if (strncmp(buffer, "P2", 2) == 0) // si el mensaje empieza con "P2"
		{
			//separar el segundo valor separado por coma usando strtok
			char* token = strtok(buffer + 3, ","); // saltar "P2,"
			token = strtok(nullptr, ","); // saltar x
			if (token != nullptr)
			{
				player2Pos.y = atof(token);
				//LOG("Jugador 2 pos Y: " << player2Pos.y);
			}
		}
	}
	

	//Por convencion, el servidor es el jugador 1 y está a la izquierda de la pantalla
	if (IsKeyDown(KEY_W) && player1Pos.y > 0 )
	{
		player1DesVel.y = -playerSpeed;
	}
	else if (IsKeyDown(KEY_S) && player1Pos.y + playerHeight < GetScreenHeight())
	{
		player1DesVel.y = playerSpeed;
	}
	else
	{
		player1DesVel.y = 0;
	}
	//un simple lerp para suavizar el movimiento
	player1Vel.y += (player1DesVel.y - player1Vel.y) * 10 * GetFrameTime();
	player1Pos.y += player1Vel.y * GetFrameTime();
	//LOG("vel: " << player1Vel.y);
	//la posicion del jugador 2 se actualiza con lo que envía el cliente

	//enviar al cliente la posición del jugador 1 y la bola

	//Calcular el movimiento de la pelota
	ballPos.x += ballVel.x * GetFrameTime();
	ballPos.y += ballVel.y * GetFrameTime();

	//Colision con las paredes de arriba y abajo
	if (ballPos.y <= 20 || ballPos.y >= GetScreenHeight() - 20) {
		ballVel.y *= -1;
	}

	//Colision con las paredes de izquierda y derecha
	if (ballPos.x < 20) {
		ballVel.x *= -1;
		player2Score += 1;
		strp2s = std::to_string(player2Score);
	}
	if( ballPos.x >= GetScreenWidth() - 20) {
		ballVel.x *= -1;
		player1Score += 1;
		strp1s = std::to_string(player1Score);
	}

	//Colision con las paletas
	if (ballPos.x +20 > player1Pos.x && ballPos.x -20 < player1Pos.x + playerWidth) {
		if (ballPos.y + 20 > player1Pos.y && ballPos.y -20 < player1Pos.y + playerHeight) {
			ballVel.x *= -1;
		}
	}
	if (ballPos.x +20 > player2Pos.x && ballPos.x -20 < player2Pos.x + playerWidth) {
		if (ballPos.y + 20 > player2Pos.y && ballPos.y -20 < player2Pos.y + playerHeight) {
			ballVel.x *= -1;
		}
	}

	if( remoteAddress != nullptr)
	{
		sprintf(buffer, "P1,%f,%f,%f", ballPos.x, ballPos.y, player1Pos.y);
		if (!network->sendTo(socket, buffer, remoteAddress))
		{
			LOG("Error al enviar datos al cliente");
			return;
		}
	}
	else
	{
		LOG("remote address null, cliente no conectado");
	}

	
	

}


void CreateClient()
{
	LOG("Creando cliente");
	//posiciones iniciales para evitar errores
	player1Pos = { 0,0 };
	player1Vel = { 0, 0 };
	player2Pos = { (float)GetScreenWidth() - 40, (float)GetScreenHeight() / 2 - playerHeight / 2 };
	player2Vel = { 0, 0 };
	ballPos = { 0,0 };

	socket = network->createSocket();
	network->setNonBlocking(socket);
	if (socket == nullptr)
	{
		LOG("Error al crear socket");
		return;
	}
	AppState = EAPPState::CLIENT;
	//DisplayMessage(true, "Conectando al servidor...");
	LOG("Cliente conectado " << PORT);
	
}

bool directInput = true;
void UpdateClient()
{
	//primero recibimos los datos del servidor
	char buffer[BUFFLEN];
	AddressHandle remoteAddress = nullptr;


	//debemos procesar todos los mensajes recibidos para evitar lag
	while (network->receiveFrom(socket, buffer, BUFFLEN, remoteAddress))
	{
		if (strncmp(buffer, "P1", 2) == 0) // si el mensaje empieza con "P1"
		{
			LOG("Recibido: " << buffer);
			//separar el segundo valor separado por coma usando strtok
			char* token = strtok(buffer + 3, ","); // saltar "P1,"
			ballPos.x = atof(token);
			token = strtok(nullptr, ","); // saltar x
			ballPos.y = atof(token);
			token = strtok(nullptr, ","); // saltar y
			player1Pos.y = atof(token);
		}
	}

	if (ballPos.x < 20) {
		player2Score += 1;
		strp2s = std::to_string(player2Score);
	}
	if (ballPos.x >= GetScreenWidth() - 20) {
		player1Score += 1;
		strp1s = std::to_string(player1Score);
	}

	//Por convencion, el cliente es el jugador 2 y está a la derecha
	if (IsKeyDown(KEY_W) && player2Pos.y > 0)
	{
		player2DesVel.y = -playerSpeed;
	}
	else if (IsKeyDown(KEY_S) && player2Pos.y + playerHeight < GetScreenHeight())
	{
		player2DesVel.y = playerSpeed;
	}
	else
	{
		player2DesVel.y = 0;
	}

	if (directInput)
	{
		//actualizar la posición del jugador 2 directamente
		player2Vel.y += (player2DesVel.y - player2Vel.y) * 10 * GetFrameTime();
		player2Pos.y += player2Vel.y * GetFrameTime();
	}
	//en realidad no se necesita enviar X pero igual la serializamos
	sprintf(buffer, "P2,%f,%f", player2Pos.x, player2Pos.y);
	network->sendTo(socket, buffer, "127.0.0.1", PORT);
}

void DrawServer()
{
	ClearBackground(BLACK);
	//barra o raqueta del jugador 1
	DrawRectangle(player1Pos.x, player1Pos.y, playerWidth, playerHeight, RAYWHITE);
	//barra o raqueta del jugador 2
	DrawRectangle(player2Pos.x, player2Pos.y, playerWidth, playerHeight, RAYWHITE);
	//bola
	DrawCircle(ballPos.x, ballPos.y, 20, RAYWHITE);

	DrawText("Servidor", 50, 10, 20, DARKGRAY);

	DrawText(strp1s.c_str(), (GetScreenWidth() / 2) - 100, 10, 40, RAYWHITE);

	DrawText(strp2s.c_str(), (GetScreenWidth() / 2) + 60, 10, 40, RAYWHITE);
}

void DrawClient()
{
	ClearBackground(BLACK);
	//barra o raqueta del jugador 1
	DrawRectangle(player1Pos.x, player1Pos.y, playerWidth, playerHeight, RAYWHITE);
	//barra o raqueta del jugador 2
	DrawRectangle(player2Pos.x, player2Pos.y, playerWidth, playerHeight, RAYWHITE);
	//bola
	DrawCircle(ballPos.x, ballPos.y, 20, RAYWHITE);
	DrawText("Cliente", 50, 10, 20, DARKGRAY);

	DrawText(strp1s.c_str(), (GetScreenWidth() / 2) - 100, 10, 40, RAYWHITE);

	DrawText(strp2s.c_str(), (GetScreenWidth() / 2) + 60, 10, 40, RAYWHITE);
	//mostrar mensaje misc
	if( screenMsg!= nullptr && screenMsg[0] != '\0' )
	{
		int w = MeasureText(screenMsg, 24);
		DrawText(screenMsg, GetScreenWidth() / 2 - w / 2, GetScreenHeight() * 0.5f, 24, RAYWHITE);
	}
}

void UpdateMenu()
{
	if (IsKeyPressed(KEY_ONE))
	{
		CreateServer();
		return;
	}
	if (IsKeyPressed(KEY_TWO))
	{
		CreateClient();
		return;
	}
}

const char* appTitle = "PONG";
const char* appDesc1 = "1 - Crear Servidor";
const char* appDesc2 = "2 - Conectar a servidor";
void DrawMenu()
{
	ClearBackground(LIGHTGRAY);
	int wA = MeasureText(appTitle, 28);
	int wB = MeasureText(appDesc1, 20);
	int wC = MeasureText(appDesc2, 20);
	DrawText(appTitle, GetScreenWidth() / 2 - wA / 2, GetScreenHeight() * 0.1f, 28, DARKGRAY);
	DrawText(appDesc1, GetScreenWidth() / 2 - wB / 2, GetScreenHeight() * 0.3f, 20, DARKGRAY);
	DrawText(appDesc2, GetScreenWidth() / 2 - wC / 2, GetScreenHeight() * 0.4f, 20, DARKGRAY);
}

void DisplayMessage(bool bShow, const char* msg)
{
	if (screenMsg == nullptr)
	{
		screenMsg = new char[100];
	}
	if (bShow)
	{
		strncpy(screenMsg, msg, 99);
		screenMsg[99] = '\0'; 
	}
	else
	{
		screenMsg[0] = '\0'; 
	}
}