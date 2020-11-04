#include "ModuleNetworkingServer.h"




//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	listenSocket = socket(AF_INET, SOCK_STREAM,0);
	// - Set address reuse
	struct sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	// - Bind the socket to a local interface
	if (bind(listenSocket, (const struct sockaddr*)&bindAddr, sizeof(bindAddr)) == NO_ERROR)
		LOG("SUCCESSFULLY binded Server Socket");
	else
		LOG("Error &d creating Server Socket", WSAGetLastError());

	// - Enter in listen mode
	if (listen(listenSocket, 24) == NO_ERROR)
		LOG("SUCCESSFULLY set up listen Server Socket");
	else
		LOG("Error: &d setting up listen Server Socket", WSAGetLastError());

	//Set socket to listen with a max of 24 foreign connections
	// - Add the listenSocket to the managed list of sockets using addSocket()
	addSocket(listenSocket);


	state = ServerState::Listening;

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}

//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	//TODO: 0 (JESUS)   

	ClientMessage clientMessage;
	packet >> clientMessage;

	if (clientMessage == ClientMessage::Hello)
	{
		std::string playerName;
		packet >> playerName;

		bool is_name_duplicate = false;
		for (auto& connectedSocket : connectedSockets)
			if (connectedSocket.playerName == playerName)
				is_name_duplicate = true;

		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.socket == socket)
			{
				//If the name is duplicate, disconnect socket and send non welcome packet, then break
				if (is_name_duplicate == true)
				{
					onUsernameTaken(connectedSocket.socket, playerName);
					break;
				}
				connectedSocket.playerName = playerName;

				// (TODO 1 DC )Send Welcome Packet
				std::string message = "Welcome to Chad's Cool Chat! You have entered a chat room.";
				OutputMemoryStream welcome_packet;
				welcome_packet << ServerMessage::Welcome;
				welcome_packet << message;

				sendPacket(welcome_packet, socket);

				//Now lets tell everyone a user joined the chat
				userJoinedOrLeft(connectedSocket.playerName, UserConnection::Joined);

				//Assuming we are done 
				break;
			}
		}
	}
	else if (clientMessage == ClientMessage::ChatMessage)
	{
		//We have a public message, send to all users
		std::string message;
		std::string username;
		packet >> message;
		packet >> username;

		OutputMemoryStream chat_packet;
		chat_packet << ServerMessage::ChatMessage;
		chat_packet << message;
		chat_packet << username;

		for (auto& connectedSocket : connectedSockets)
		{
			//If the socket is not the listen socket, send them the public chat message
			if (connectedSocket.socket != this->listenSocket)
				sendPacket(chat_packet, connectedSocket.socket);
		}
		
	}
	else if (clientMessage == ClientMessage::Whisper)
	{
		//Same as message but we only send the message to the destined user
		std::string message;
		std::string username;
		std::string dest_username;

		packet >> message;
		packet >> username;
		packet >> dest_username;

		OutputMemoryStream chat_packet;
		chat_packet << ServerMessage::Whisper;
		chat_packet << message;
		chat_packet << username;
		chat_packet << dest_username;

		if (userIsConnected(dest_username))
		{
			for (auto& connectedSocket : connectedSockets)
			{
				//If the socket is not the listen socket, send them the public chat message
				if (connectedSocket.socket != this->listenSocket
					&& connectedSocket.playerName == username || connectedSocket.playerName == dest_username)
					sendPacket(chat_packet, connectedSocket.socket);
			}
		}
	}
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto &connectedSocket = *it;
		if (connectedSocket.socket == socket)
		{
			//Now lets tell everyone a user left the chat
			userJoinedOrLeft(connectedSocket.playerName, UserConnection::Left);
			connectedSockets.erase(it);
			break;
		}
	}
}

void ModuleNetworkingServer::userJoinedOrLeft(std::string& username, UserConnection connection_state)
{
	OutputMemoryStream packet;
	packet << ServerMessage::UserEvent;
	packet << connection_state;
	packet << username;

	for (auto& connectedSocket : connectedSockets)
	{
		//If the socket is not the listen socket, send them the public joined_left packet
		if (connectedSocket.socket != this->listenSocket)
			sendPacket(packet, connectedSocket.socket);
	}
}

void ModuleNetworkingServer::onUsernameTaken(SOCKET socket, const std::string &username)
{
	std::string message = "Your username is already taken, please change it and retry connection to the server to access the chat.";
	OutputMemoryStream packet;
	ServerMessage msg_type = ServerMessage::NonWelcome;

	packet << msg_type;
	packet << message;

	sendPacket(packet, socket);

	LOG("Socket %d  with username %s disconnected! Forced disconnection due to duplicity of usernames.", socket, username.c_str());
	

	//Disconnect the socket
	onSocketDisconnected(socket);

	for (std::vector<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); it++)
	{
		if ((*it) == socket)
		{
			sockets.erase(it);
			break;
		}
	}
}

bool ModuleNetworkingServer::userIsConnected(std::string &username)
{
	for (auto& connectedSocket : connectedSockets)
	{
		if (connectedSocket.playerName.compare(username) == 0) // If they compare equal (on success compare returns 0)
		{
			return true;
		}
	}

	return false;
}

