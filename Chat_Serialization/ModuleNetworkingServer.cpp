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

		bool refuse_connection = false;
		std::string refusal_reason;

		// Check for a repeated name
		for (auto& connectedSocket : connectedSockets)
			if (connectedSocket.playerName == playerName) {
				refusal_reason = "Your username is already taken, please change it and retry connection to the server to access the chat.";
				refuse_connection = true;
			}

		// Check for a user ban
		for (std::string& user_ban : banned_users)
			if (user_ban == playerName) {
				refusal_reason = "You have been banned from this chat. Ask another user to unban you.";
				refuse_connection = true;
			}

		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.socket == socket)
			{
				//If the name is duplicate, disconnect socket and send non welcome packet, then break
				if (refuse_connection == true)
				{
					onRefuseConnection(connectedSocket.socket, playerName, refusal_reason);
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
	else if (clientMessage == ClientMessage::ChatMessage || clientMessage == ClientMessage::AnonChatMessage)
	{
		//We have a public message, send to all users
		std::string message;
		std::string username;
		packet >> message;
		packet >> username;

		OutputMemoryStream chat_packet;

		if (clientMessage == ClientMessage::AnonChatMessage)
			chat_packet << ServerMessage::AnonChatMessage;
		else
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
		std::string dest_username;
		std::string message;
		std::string username;

		packet >> message;
		packet >> username;
		packet >> dest_username;

		OutputMemoryStream chat_packet;
		chat_packet << ServerMessage::Whisper;

		if (userIsConnected(dest_username))
		{
			chat_packet << true;	// This marks the whisper as succesful
			chat_packet << message;
			chat_packet << username;
			chat_packet << dest_username;

			for (auto& connectedSocket : connectedSockets)
			{
				//If the socket is not the listen socket, send them the public chat message
				if (connectedSocket.socket != this->listenSocket
					&& connectedSocket.playerName == username || connectedSocket.playerName == dest_username)
					sendPacket(chat_packet, connectedSocket.socket);
			}
		}
		else {
			chat_packet << false;				// This marks the whisper as unsuccesful because the destination user is not connected
			sendPacket(chat_packet, socket);
		}
	}
	else if (clientMessage == ClientMessage::ChangeName)
	{
		std::string new_username;
		std::string old_username;

		packet >> new_username;
		packet >> old_username;

		ConnectedSocket* socket_ptr = nullptr;

		bool name_taken = false;
		for (int i = 0; !name_taken && i < connectedSockets.size(); ++i) {	//Check that the name is not taken by another user
			if (connectedSockets[i].socket != this->listenSocket) {
				if (connectedSockets[i].socket == socket) {		// Locate & Save client who requested the namechange in the server list
					socket_ptr = &connectedSockets[i];
				}
				else if (connectedSockets[i].playerName == new_username) {
					name_taken = true;
				}
			}
		}

		OutputMemoryStream name_packet;
		name_packet << ServerMessage::ChangeName;
		name_packet << name_taken;

		if (!name_taken) {	//If the new username is not taken, we change his name on the server user list and send a packet to everyone stating the name change of the user who made the request
			socket_ptr->playerName = new_username;
			name_packet << new_username;
			name_packet << old_username;

			for (auto& connectedSocket : connectedSockets)
				if (connectedSocket.socket != this->listenSocket)
					sendPacket(name_packet, connectedSocket.socket);
		}
		else {				//Otherwise, we send to the user who made the request an "operation failed" flag because of the taken username
			sendPacket(name_packet, socket);
		}
	}
	else if (clientMessage == ClientMessage::List)
	{
		OutputMemoryStream list_packet;
		list_packet << ServerMessage::List;
		list_packet << (unsigned int)connectedSockets.size();

		for (auto& connectedSocket : connectedSockets)	// Serialize all user names
		{
			list_packet << connectedSocket.playerName;
		}

		sendPacket(list_packet, socket);
	}
	else if (clientMessage == ClientMessage::Mute)
	{
		muteRequest(socket, packet, ServerMessage::Mute);
	}
	else if (clientMessage == ClientMessage::UnMute)
	{
		muteRequest(socket, packet, ServerMessage::UnMute);
	}
	else if (clientMessage == ClientMessage::Kick)
	{
		std::string kicked_username;
		packet >> kicked_username;

		OutputMemoryStream kick_packet;
		kick_packet << ServerMessage::Kick;
		
		bool player_found = false;
		for (auto& connectedSocket : connectedSockets)
		{
			//If the socket has the same username, send kick 
			if (connectedSocket.playerName == kicked_username)
			{
				player_found = true;
				kick_packet << true;	//Flag to mark operation succesful
				sendPacket(kick_packet, connectedSocket.socket);
				onSocketDisconnected(connectedSocket.socket);
				break;
			}
		}

		if (!player_found) {
			kick_packet << false;
			sendPacket(kick_packet, socket);
		}
	}
	else if (clientMessage == ClientMessage::MuteList)
	{
		OutputMemoryStream mutelist_packet;
		mutelist_packet << ServerMessage::MuteList;
		mutelist_packet << (unsigned int)muted_users.size();
		
		for (std::string& m_user : muted_users)	// Serialize all user names
		{
			mutelist_packet << m_user;
		}

		sendPacket(mutelist_packet, socket);
	}
	else if (clientMessage == ClientMessage::Ban)
	{
		banRequest(socket, packet, ServerMessage::Ban);
	}
	else if (clientMessage == ClientMessage::UnBan)
	{
		banRequest(socket, packet, ServerMessage::UnBan);
	}
	else if (clientMessage == ClientMessage::BanList)
	{
		OutputMemoryStream banlist_packet;
		banlist_packet << ServerMessage::BanList;
		banlist_packet << (unsigned int)banned_users.size();

		for (std::string& m_user : banned_users)	// Serialize all user names
		{
			banlist_packet << m_user;
		}

		sendPacket(banlist_packet, socket);
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

void ModuleNetworkingServer::onRefuseConnection(SOCKET socket, const std::string &username, std::string& reason)
{
	OutputMemoryStream packet;
	ServerMessage msg_type = ServerMessage::NonWelcome;

	packet << msg_type;
	packet << reason;

	sendPacket(packet, socket);

	LOG("Socket %d with username %s is forced to disconnect! Banned or duplicated username!", socket, username.c_str());

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

bool ModuleNetworkingServer::muteRequest(SOCKET socket, const InputMemoryStream& packet, ServerMessage mute_or_unmute)
{
	std::string muted_user;
	std::string mute_mode;

	packet >> muted_user;
	packet >> mute_mode;

	OutputMemoryStream mute_packet;
	mute_packet << mute_or_unmute;	// Mark mute purpose, if mute or unmute

	bool user_found = false;

	for (auto& connectedSocket : connectedSockets)
	{
		//If the socket has the same username, we confirm it exists
		if (connectedSocket.playerName == muted_user)
		{
			user_found = true;

			mute_packet << true;		// Flag user exists
			mute_packet << muted_user;	// Mute target
			mute_packet << mute_mode;	// Mute mode

			if (mute_mode == "local") {	// If local, we tell sender client we found muted user exists
				sendPacket(mute_packet, socket);
			}
			else {	// If global

				bool muted_user_registered = false;

				for (std::vector<std::string>::iterator it = muted_users.begin(); it != muted_users.end(); ++it) {
					if ((*it) == muted_user) {
						
						muted_user_registered = true;	// Flag that the user is inside the server mute list

						if (mute_or_unmute == ServerMessage::UnMute)	// Remove from list if the order is to UNMUTE
							muted_users.erase(it);

						break;
					}
				}

				if (mute_or_unmute == ServerMessage::Mute && !muted_user_registered) {	// If the order is to MUTE and he's not on the list, add it
					muted_users.push_back(muted_user);
				}

				// Tell all clients about the global mute
				sendPacket(mute_packet, connectedSocket.socket);
			}

			break;
		}
	}

	if (!user_found) {
		mute_packet << false;	// Flag that user was not found, return to sender AND ONLY SENDER
		sendPacket(mute_packet, socket);
	}

	return true;
}

bool ModuleNetworkingServer::banRequest(SOCKET socket, const InputMemoryStream& packet, ServerMessage ban_or_unban)	//NOTE: A ban done by name is not great, but we'll keep it simple
{
	std::string banned_user;
	packet >> banned_user;

	OutputMemoryStream ban_packet;
	ban_packet << ban_or_unban;	// Mark ban purpose, if ban or unban
	ban_packet << banned_user;	// Mark ban target

	bool banned_user_registered = false;

	for (std::vector<std::string>::iterator it = banned_users.begin(); it != banned_users.end(); ++it) {
		if ((*it) == banned_user) {

			banned_user_registered = true;	// Flag that the user is inside the server ban list

			if (ban_or_unban == ServerMessage::UnBan)	// Remove from list if the order is to UNBAN
				banned_users.erase(it);

			break;
		}
	}

	if (ban_or_unban == ServerMessage::Ban && !banned_user_registered) {	// If the order is to BAN and he's not on the list, add it
		banned_users.push_back(banned_user);
	}

	for (auto& connectedSocket : connectedSockets)	//NOTE: Because a BAN can be done to an "offline user", we don't care if the user isn't connected, but if he is he will be KICKED as well
	{
		// Tell all clients about the ban
		sendPacket(ban_packet, connectedSocket.socket);
	}

	return true;
}