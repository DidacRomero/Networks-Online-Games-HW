#include "ModuleNetworkingClient.h"

// By Dídac Romero & Carles Homs

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
		// TODO: 0 (JESUS) prepare the packet
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, socket))
		{
			state = ClientState::Logging;
		}
		else
		{
			disconnect();
			state = ClientState::Stopped;
			return true;
		}
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

		ImGui::Text("Hello %s, welcome to the Chat!", playerName.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Logout")) {
			disconnect();
			state = ClientState::Stopped;
		}

		ImGui::Spacing();

		// ORIGINAL

		ImGui::BeginChild("MessageRegion", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), true);

		//ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::TextColored(ImVec4(255, 255, 0, 255), "      ****************************************\n                WELCOME TO THE CHAT\n        Type /help for the list of commands!\n      ****************************************");
		ImGui::PushTextWrapPos(ImGui::GetWindowWidth());

		if (messages.size() > 0) {
			int a = 0;
		}

		for (auto& msg : messages) {
			if (msg.is_whisper) {
				if (msg.username == playerName) // If this client is the sender
					ImGui::TextColored(ImVec4(0, 255, 255, 255), "You whispered to %s: %s", msg.whispered_user.c_str(), msg.message.c_str());
				else
					ImGui::TextColored(ImVec4(0, 255, 255, 255), "%s whispered to You: %s", msg.username.c_str(), msg.message.c_str());
			}
			else if (msg.is_system) {	// IF it's a system message
				ImGui::TextColored(ImVec4(255, 255, 0, 255), "%s", msg.message.c_str());
			}
			else if (msg.is_anonymous) {	// We make the display of the message anonymous to other users and mark it as so for the sender,
											// but the sender username is still stored along the message for everyone if we want this linking information for other purposes
				if (msg.username == playerName)
					ImGui::Text("%s (as Anonymous): %s", msg.username.c_str(), msg.message.c_str());
				else
					ImGui::Text("(Anonymous): %s", msg.message.c_str());
			}
			else {
				ImGui::Text("%s: %s", msg.username.c_str(), msg.message.c_str());
			}
		}
		ImGui::PopTextWrapPos();

		if (new_message) {	// If new message sent or recieved, scroll down immediately	//IMPROVE: Ideally, if recieved we should only scroll if we were already at the bottom to not interrupt reading of prev. messages
			ImGui::SetScrollHereY(1.0f);
			new_message = false;
		}

		ImGui::EndChild();

		/*ImGui::Spacing();
		ImGui::Dummy(ImVec2(0.0f, ImGui::GetWindowHeight() - 330.0f));*/

		//static char str1[128] = "";
		//if (ImGui::InputTextWithHint("", "Type a message...", str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_EnterReturnsTrue))
		//{
		//	// Send the PUBLIC message here
		//	std::string str_message = str1;
		//	sendChatMessage(str_message);
		//}

		//// Auto-focus on window apparition
		//ImGui::SetItemDefaultFocus();
		//if (true)
		//	ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

		static char str1[128] = "";
		if (ImGui::InputTextWithHint("", "Press Enter to send your message!", str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			// Send the PUBLIC message here
			std::string str_message = str1;
			if (str_message.size() > 0) {

				strcpy_s(str1, strlen(str1), "");

				// If a / is found, search for a command
				if (str_message.find_first_of("/") != std::string::npos)
				{
					if (str_message.find("/help") != std::string::npos)
					{
						ChatMessage help_msg;
						help_msg.message = " ****** Commands List ******\n /help\n /list\n /changename <newname>\n /anonymous <message>\n /whisper <username> <message>\n /(un)mute <local/global> <username> \n /mutelist \n /kick <username>\n /(un)ban <username> \n /banlist \n /clear \n /logout | /exit | /quit";
						help_msg.is_system = true;

						messages.push_back(help_msg);
						new_message = true;
					}
					else if (str_message.find("/list") != std::string::npos)
					{
						OutputMemoryStream packet;
						packet << ClientMessage::List;	// Message type

						sendPacket(packet, socket);
					}
					else if (str_message.find("/anonymous ") != std::string::npos)
					{
						OutputMemoryStream packet;
						packet << ClientMessage::AnonChatMessage;

						str_message.erase(0, 11);

						if (!str_message.empty()) {	// We check that a message has been given
							packet << str_message;	//Message
							packet << playerName;	//Username that sent the message

							sendPacket(packet, socket);
						}
						else {
							WLOG("You didn't input any anonymous message!");
						}
					}
					else if (str_message.find("/changename ") != std::string::npos)
					{
						// We delete the first part: "/changename "
						str_message.erase(0, 12);

						if (!str_message.empty()) {	// We check that a new username has been given
							OutputMemoryStream packet;
							packet << ClientMessage::ChangeName;
							packet << str_message;	// Message
							packet << playerName;	// Username that sent the message

							sendPacket(packet, socket);
						}
						else {
							WLOG("You didn't input any new username!");
						}
					}
					else if (str_message.find("/whisper ") != std::string::npos)
					{
						std::string dest_username;
						std::size_t user_end;

						str_message.erase(0, 9);	//Eliminate the command /whisper

						//Find the next space (where the username ends)
						user_end = str_message.find(" ");

						if (user_end != std::string::npos && user_end != 0) {	//Check that formatting is correct
							dest_username = str_message.substr(0, user_end);	//Record destination username
							str_message.erase(0, user_end + 1);					//Delete username and the space separating it from the message

							if (str_message.size() > 0) {
								if (dest_username != playerName)
									sendChatMessage(str_message, true, dest_username);
								else
									WLOG("Whispering yourself is kinda pointless.");
							}
							else
								WLOG("No message given to whisper!");
						}
						else {
							WLOG("Incorrect use of whisper command! Expected format: /whisper <username> <message>");
						}
					}
					else if (str_message.find("/mute ") != std::string::npos)	//Mute a user
					{
						str_message.erase(0, 6);	//Eliminate the command /mute
						sendMute(str_message, true);
					}
					else if (str_message.find("/unmute ") != std::string::npos)	//Unmute a user
					{
						str_message.erase(0, 8);	//Eliminate the command /unmute
						sendMute(str_message, false);
					}
					else if (str_message.find("/mutelist") != std::string::npos)
					{
						OutputMemoryStream packet;
						packet << ClientMessage::MuteList;	// Message type

						sendPacket(packet, socket);
					}
					else if (str_message.find("/kick ") != std::string::npos)
					{
						// We delete the first part: "/kick "
						str_message.erase(0, 6);

						if (!str_message.empty()) {	// We check that a new username has been given
							OutputMemoryStream packet;
							packet << ClientMessage::Kick;
							packet << str_message;	// Message

							sendPacket(packet, socket);
						}
						else {
							WLOG("You didn't input any username to kick!");
						}
					}
					else if (str_message.find("/ban ") != std::string::npos)	//Ban a user
					{
						str_message.erase(0, 5);	//Eliminate the command /ban
						sendBan(str_message, true);
					}
					else if (str_message.find("/unban ") != std::string::npos)	//Unban a user
					{
						str_message.erase(0, 7);	//Eliminate the command /unban
						sendBan(str_message, false);
					}
					else if (str_message.find("/banlist") != std::string::npos)
					{
						OutputMemoryStream packet;
						packet << ClientMessage::BanList;	// Message type

						sendPacket(packet, socket);
					}
					else if (str_message.find("/clear") != std::string::npos)
					{
						messages.clear();
					}
					else if (str_message.find("/logout") != std::string::npos || str_message.find("/exit") != std::string::npos || str_message.find("/quit") != std::string::npos)
					{
						disconnect();
						state = ClientState::Stopped;
					}
				}
				else
				{
					sendChatMessage(str_message);
				}
			}

			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget (Mark input text to not unselect after sending a message
		}

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	//We will do different actions depensing on the type of message
	ServerMessage message_received;
	packet >> message_received;

	if (message_received == ServerMessage::Welcome)	//In case we are welcomed by the server (just connected)
	{
		std::string welcome_message;
		packet >> welcome_message;

		LOG("%s", welcome_message.c_str());

		ChatMessage help_msg;
		help_msg.message = welcome_message;
		help_msg.is_system = true;

		messages.push_back(help_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::NonWelcome)
	{
		std::string reason;
		packet >> reason;

		WLOG("You were automatically kicked from the server. Reason: %s.", reason.c_str());
		state = ClientState::Stopped;
	}
	else if (message_received == ServerMessage::ChatMessage || message_received == ServerMessage::AnonChatMessage)
	{
		ChatMessage chat_message;
		packet >> chat_message.message;
		packet >> chat_message.username;

		bool muted_user = false;

		//If the message is anonymous, censor the username for all clients which aren't the sender of the message
		if (message_received == ServerMessage::AnonChatMessage)
			chat_message.is_anonymous = true;

		if (!chat_message.is_anonymous) {	// If not anonymous, check if muted
			for (std::string user : local_mutes) {	// Check if user is muted locally
				if (user == chat_message.username) {
					muted_user = true;
					break;
				}
			}
		}

		if (!muted_user) {	// If user muted

			//Add the public message to our messages list
			messages.push_back(chat_message);
			new_message = true;	// Mark that a new message has been recieved! We will autoscroll down if we were

			//LOG to test
			DLOG("Test Log:   %s : %s", chat_message.username.c_str(), chat_message.message.c_str());
		}
		else {
			DLOG("(MUTED) %s: %s", chat_message.username.c_str(), chat_message.message.c_str());
		}
	}
	else if (message_received == ServerMessage::Whisper)
	{
		bool dest_user_exists = true;
		packet >> dest_user_exists;

		if (dest_user_exists) {
			ChatMessage chat_message;
			chat_message.is_whisper = true;
			packet >> chat_message.message;
			packet >> chat_message.username;
			packet >> chat_message.whispered_user;

			bool muted_user = false;
			for (std::string user : local_mutes) {	// Check if user is muted locally
				if (user == chat_message.username) {
					muted_user = true;
					break;
				}
			}

			if (!muted_user) {
				//Add the public message to our messages list
				messages.push_back(chat_message);
				new_message = true;	// Mark that a new message has been recieved! We will autoscroll down if we were

				//LOG to test
				DLOG("%s whispered to You: %s", chat_message.username.c_str(), chat_message.message.c_str());
			}
			else {
				WLOG("%s tried whispering to You, but you locally MUTED him. Message: %s", chat_message.username.c_str(), chat_message.message.c_str());
			}
		}
		else {
			WLOG("Whispered user is not connected!");
		}
	}
	else if (message_received == ServerMessage::ChangeName)
	{
		bool taken_username = false;
		packet >> taken_username;

		if (taken_username)
			WLOG("Username requested is taken by another connected user!");
		else {
			std::string new_username;
			std::string old_username;

			packet >> new_username;
			packet >> old_username;

			for (ChatMessage& msg : messages) {
				if (msg.username == old_username) {
					msg.username = new_username;
				}
			}
			if (playerName == old_username) { playerName = new_username; }
		}
	}
	else if (message_received == ServerMessage::UserEvent)
	{
		UserConnection connection_state;
		packet >> connection_state;
		std::string username;
		packet >> username;

		ChatMessage chat_msg;
		chat_msg.is_system = true;

		if (connection_state == UserConnection::Joined) {
			chat_msg.message = std::string("User " + username + " joined the chat.");
		}
		else if (connection_state == UserConnection::Left) {
			chat_msg.message = std::string("User " + username + " left the chat.");
		}
		else if (connection_state == UserConnection::Aborted) {
			std::string abort_reason;
			packet >> abort_reason;
			chat_msg.message = std::string("User " + username + " tried to join the chat, but was automatically kicked. Reason: " + abort_reason);
		}

		DLOG(chat_msg.message.c_str());

		messages.push_back(chat_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::List)
	{
		ChatMessage list_message;
		list_message.is_system = true;
		list_message.message = " ****** Connected Users ******";
		unsigned int num_users = 0;

		packet >> num_users;
		for (unsigned int i = 0; i < num_users; ++i) {
			std::string username;
			packet >> username;
			list_message.message.append("\n - " + username);
		}
		messages.push_back(list_message);
		new_message = true;
	}
	else if (message_received == ServerMessage::Mute)
	{
		bool user_found = true;
		packet >> user_found;

		ChatMessage chat_msg;
		chat_msg.is_system = true;

		if (user_found) {
			std::string muted_user;
			std::string mute_mode;

			packet >> muted_user;
			packet >> mute_mode;

			if (mute_mode == "local") {	//If it's a local mute, we add him to our local mute list
				local_mutes.push_back(muted_user);
				chat_msg.message = std::string("You locally muted " + muted_user);
				DLOG(chat_msg.message.c_str());
			}
			else {
				if (muted_user == playerName) {	//If it's global, we check if we are the target
					globally_muted = true;	//If we are, we mark ourselves as muted
					chat_msg.message = std::string("You have been GLOBALLY MUTED!");
				}
				else {
					chat_msg.message = std::string(muted_user + " has been GLOBALLY MUTED!");
				}

				DLOG(chat_msg.message.c_str());
			}
		}
		else {	//If user was not found, it means we are the sender of the original message and we got a response from the server
			chat_msg.message = std::string("The user you tried to mute is not connected or has changed names!");
			WLOG(chat_msg.message.c_str());
		}

		messages.push_back(chat_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::UnMute)
	{
		bool user_found = true;
		packet >> user_found;

		ChatMessage chat_msg;
		chat_msg.is_system = true;

		if (user_found) {
			std::string muted_user;
			std::string mute_mode;

			packet >> muted_user;
			packet >> mute_mode;

			if (mute_mode == "local") {	//If it's a local mute, we search and remove him from our local mute list
				for (std::vector<std::string>::iterator it = local_mutes.begin(); it != local_mutes.end(); ++it) {
					if ((*it) == muted_user) {
						local_mutes.erase(it);
						chat_msg.message = std::string("You LOCALLY UNMUTED " + muted_user + " !");
						DLOG(chat_msg.message.c_str());
						break;
					}
				}
			}
			else {
				if (muted_user == playerName) {	//If it's global, we check if we are the target
					globally_muted = false;	//If we are, we mark ourselves as unmuted
					chat_msg.message = std::string("You have been GLOBALLY UNMUTED!");
				}
				else {
					chat_msg.message = std::string(muted_user + " has been GLOBALLY UNMUTED!");
				}

				DLOG(chat_msg.message.c_str());
			}
		}
		else {	//If user was not found, it means we are the sender of the original message and we got a response from the server
			chat_msg.message = std::string("The user you tried to unmute is not connected or has changed names!");
			WLOG(chat_msg.message.c_str());
		}

		messages.push_back(chat_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::MuteList)
	{
		ChatMessage mute_list_msg;
		mute_list_msg.message = " ****** Locally Muted Users ******";

		for (std::string& muted_user : local_mutes)
			mute_list_msg.message.append("\n - " + muted_user);

		mute_list_msg.message.append("\n\n ****** Globally Muted Users ******");

		unsigned int global_list_size = 0;
		packet >> global_list_size;

		for (unsigned int i = 0; i < global_list_size; ++i) {
			std::string muted_user;
			packet >> muted_user;
			mute_list_msg.message.append("\n - " + muted_user);
		}

		mute_list_msg.is_system = true;

		messages.push_back(mute_list_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::Kick)
	{
		bool kick_success = false;
		packet >> kick_success;
		
		ChatMessage chat_msg;
		chat_msg.is_system = true;
		
		if (kick_success) {
			std::string kicked_user;
			packet >> kicked_user;

			if (kicked_user == playerName) {
				WLOG("You've been KICKED!");
				disconnect();
				state = ClientState::Stopped;
			}
			else {
				chat_msg.message = std::string(kicked_user + " has been KICKED!");
				WLOG("%s has been KICKED!", kicked_user.c_str());

				messages.push_back(chat_msg);
				new_message = true;
			}
		}
		else {
			chat_msg.message = std::string("The user you tried to kick is not connected or has changed names!");
			WLOG(chat_msg.message.c_str());

			messages.push_back(chat_msg);
			new_message = true;
		}
	}
	else if (message_received == ServerMessage::Ban)
	{
		std::string banned_user;

		packet >> banned_user;

		ChatMessage chat_msg;
		chat_msg.is_system = true;

		if (banned_user == playerName) {	//We check if we are the target
			WLOG("You have been BANNED!");
			disconnect();
			state = ClientState::Stopped;
		}
		else {
			chat_msg.message = std::string(banned_user + " has been BANNED!");
			WLOG(chat_msg.message.c_str());

			messages.push_back(chat_msg);
			new_message = true;
		}
	}
	else if (message_received == ServerMessage::UnBan)
	{
		std::string banned_user;
		packet >> banned_user;

		ChatMessage chat_msg;
		chat_msg.is_system = true;

		chat_msg.message = std::string(banned_user + " has been UNBANNED!");
		WLOG(chat_msg.message.c_str());

		messages.push_back(chat_msg);
		new_message = true;
	}
	else if (message_received == ServerMessage::BanList)
	{
		ChatMessage ban_list_msg;
		ban_list_msg.message = " ****** Banned Users ******";

		unsigned int ban_list_size = 0;
		packet >> ban_list_size;

		for (unsigned int i = 0; i < ban_list_size; ++i) {
			std::string muted_user;
			packet >> muted_user;
			ban_list_msg.message.append("\n - " + muted_user);
		}

		ban_list_msg.is_system = true;

		messages.push_back(ban_list_msg);
		new_message = true;
	}
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}


// Send a message to other users
void ModuleNetworkingClient::sendChatMessage(std::string& message, bool isWhisper, std::string dest_username)
{
	if (!globally_muted || isWhisper) {	//Unable to send messages if globally muted, unless it's a whisper

		OutputMemoryStream packet;
		if (!isWhisper)
			packet << ClientMessage::ChatMessage;
		else
			packet << ClientMessage::Whisper;

		//Add infromation to the packet, WE ALWAYS WILL FOLLOW THIS FORMAT OF SERIALIZATION AND DE-SERIALIZATION
		packet << message;	//Message
		packet << playerName; //Username that sent the message

		if (isWhisper)
			packet << dest_username;

		sendPacket(packet, socket);
	}
	else {	// If globally muted and tried to send a global message, the client sends a message anyway, but no one recieves it
		ChatMessage msg;
		msg.username = playerName;
		msg.message = message;
		
		messages.push_back(msg);
		new_message = true;	// Mark that a new message has been recieved! We will autoscroll down if we were

		//LOG the fact that he's globally muted
		DLOG("(GLOBALLY MUTED) %s : %s", msg.username.c_str(), msg.message.c_str());
	}
}

void ModuleNetworkingClient::sendMute(std::string& message, bool muting)
{
	std::string mute_mode;
	std::size_t mode_end;

	//Find the next space (where the username ends)
	mode_end = message.find(" ");

	if (mode_end != std::string::npos && mode_end != 0) {	//Check that formatting is correct
		mute_mode = message.substr(0, mode_end);			//Record mute mode
		message.erase(0, mode_end + 1);						//Delete mute_mode and the space separating it from username

		if (mute_mode == "local" || mute_mode == "global") {	// Mark mute mode

			if (muting && message == playerName) {
				WLOG("Muting yourself is kinda pointless.");
			}
			else {
				OutputMemoryStream packet;

				if (muting)
					packet << ClientMessage::Mute;
				else
					packet << ClientMessage::UnMute;

				packet << message;		// Mute User Target
				packet << mute_mode;	// Mute Mode

				sendPacket(packet, socket);
			}
		}
		else {
			WLOG("Incorrect input of Mute Mode parameter. Expected <global> or <local> without <>.");
		}
	}
	else {
		WLOG("Incorrect use of (un)mute command! Expected format: /(un)mute <local/global> <username>");
	}
}

void ModuleNetworkingClient::sendKick(std::string& username)
{
	OutputMemoryStream packet;
	packet << ClientMessage::Kick;
	packet << username,

	sendPacket(packet, socket);
}

void ModuleNetworkingClient::sendBan(std::string& message, bool banning)
{
	if (banning && message == playerName) {
		WLOG("Banning yourself is not very smart, is it?");
	}
	else {
		OutputMemoryStream packet;

		if (banning)
			packet << ClientMessage::Ban;
		else
			packet << ClientMessage::UnBan;

		packet << message;		// Mute User Target

		sendPacket(packet, socket);
	}
}