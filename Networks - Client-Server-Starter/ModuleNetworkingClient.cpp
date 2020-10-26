#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	bool ret = false;
	// TODO(jesus): TCP connection stuff
	// - Create the socket
	this->socket = ::socket(AF_INET, SOCK_STREAM, 0);		//Since socket() doesn't belong to a namespace we use the global namespace ( :: )

	// - Create the remote address object
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &serverAddress.sin_addr);
	
	// - Connect to the remote address
	if (connect(socket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) == NO_ERROR)
	{
		DLOG("Connected to server SUCCESSFULLY!");
		ret = true;
	}
	else
	{
		DLOG("Error %d connecting to server", WSAGetLastError());
		closesocket(socket);
		return false;
	}


	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(socket);

	// If everything was ok... change the state
	state = ClientState::Start;
	//if (ret = true)
	//{
	//	state = ClientState::Logging;
	//}
	//else
	//{
	//	state = ClientState::Start;
	//}

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server
		if (send(socket, playerName.c_str(), sizeof(playerName), 0) != SOCKET_ERROR)
			DLOG("Sent playerName: %s correctly to server!", playerName.c_str());
		else
			DLOG("Failed to send playerName: %s correctly to server! Error: ", playerName.c_str(), WSAGetLastError());

		state = ClientState::Logging;
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, byte * data)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

