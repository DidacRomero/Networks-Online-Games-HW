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
		ImGui::Spacing();

		// ORIGINAL

		ImGui::BeginChild("MessageRegion", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), true);

		//ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::TextColored(ImVec4(255, 255, 0, 255), "      ****************************************\n                WELCOME TO THE CHAT\n        Type /help for the list of commands!\n      ****************************************");
		ImGui::PushTextWrapPos(ImGui::GetWindowWidth());

		if (messages.size() > 0) {
			int a = 0;
		}

		for (auto msg : messages) {
			if (msg.is_whisper) {
				ImGui::TextColored(ImVec4(128, 128, 128, 255), "(Whisper) %s: %s", msg.username.c_str(), msg.message.c_str());
			}
			else if (msg.username.compare("") == 0) {	// IF it's a system message
				ImGui::Text("%s", msg.message.c_str());
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
			strcpy_s(str1, strlen(str1), "");

			// If a / is found, search for a command
			if (str_message.find_first_of("/") != std::string::npos)
			{
				if (str_message.find("/whisper") != std::string::npos)
				{
					std::string dest_username;
					std::size_t first_; std::size_t second_;

					//Find the first space " " and the second space " " to extract the username between them
					first_ = str_message.find(" ");
					second_ = str_message.find(":");

					//Extract the username
					if (second_ < 128) //Make sure we are really sending a whisper, so we don't crash the app
					{
						int username_len = second_ - first_;
						char buffer[128];

						dest_username = str_message.substr(first_ +1, username_len -1);
						//std::size_t length = str_message.copy(buffer, first_ - 1, username_len);
						//buffer[username_len] = '\0';

						str_message.erase(0, second_ + 2);
						sendChatMessage(str_message, true, dest_username);
					}
					else
						WLOG("You didn't send a message to a user, the whisper won't be sent");
				}
				else if (str_message.find("/kick") != std::string::npos)
				{
					std::string banned_user;
					std::size_t first; std::size_t second;

					//Find the first space "<" and the second space ">" to extract the username between them
					first = str_message.find("<");
					second = str_message.find(">");

					int username_len = second - first;

					banned_user = str_message.substr(first + 1, username_len - 1);

					OutputMemoryStream packet;
					packet << ClientMessage::Kick;
					packet << banned_user;

					sendPacket(packet, socket);
				}
				else if (str_message.find("/help") != std::string::npos)
				{
					ChatMessage help_msg;
					help_msg.is_whisper = false;
					help_msg.username = "";
					help_msg.message = " ****** Commands List ******\n /help\n /userlist\n /whisper <username>\n /kick <username>\n /TODO\n /TODO";

					messages.push_back(help_msg);
				}
				else if (str_message.find("/list") != std::string::npos)
				{
				}
				else if (str_message.find("/change_name") != std::string::npos)
				{
				}
			}
			else
			{
				sendChatMessage(str_message);
			}

			new_message = true;

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
	}
	else if (message_received == ServerMessage::NonWelcome)
	{
		LOG("The username %s is already taken, please change your name and try again", playerName.c_str());
		state = ClientState::Stopped;
	}
	else if (message_received == ServerMessage::ChatMessage)
	{
		ChatMessage chat_message;
		packet >> chat_message.message;
		packet >> chat_message.username;

		//Add the public message to our messages list
		messages.push_back(chat_message);

		new_message = true;	// Mark that a new message has been recieved! We will autoscroll down if we were

		//LOG to test
		DLOG("Test Log:   %s : %s", chat_message.username.c_str(),chat_message.message.c_str());
	}
	else if (message_received == ServerMessage::Whisper)
	{
		ChatMessage chat_message;
		chat_message.is_whisper = true;
		packet >> chat_message.message;
		packet >> chat_message.username;
		packet >> chat_message.whispered_user;

		//Add the public message to our messages list
		messages.push_back(chat_message);

		new_message = true;	// Mark that a new message has been recieved! We will autoscroll down if we were

		//LOG to test
		DLOG("Test Log:   %s  whispered %s: %s", chat_message.username.c_str(), chat_message.whispered_user.c_str(), chat_message.message.c_str());
	}
	else if (message_received == ServerMessage::UserEvent)
	{
		UserConnection connection_state;
		packet >> connection_state;
		std::string username;
		packet >> username;

		if(connection_state == UserConnection::Joined)
		DLOG("User %s joined the chat.", username.c_str());
		else if (connection_state == UserConnection::Left)
		DLOG("User %s left the chat.", username.c_str());
	}
	else if (message_received == ServerMessage::Kick)
	{
		state = ClientState::Stopped;
	}
	// We can use this in the future if the server sends a serverMessage::BANNED or something like this
	//state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}


// Send a message to other users
void ModuleNetworkingClient::sendChatMessage(std::string& message, bool isWhisper, std::string dest_username)
{
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

	sendPacket(packet,socket);
}

void ModuleNetworkingClient::sendKick(std::string& username)
{
	OutputMemoryStream packet;
	packet << ClientMessage::Kick;
	packet << username,

	sendPacket(packet, socket);
}

