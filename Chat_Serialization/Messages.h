#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	ChatMessage,
	Whisper,
	Kick
};

enum class ServerMessage
{
	Welcome,
	NonWelcome,
	ChatMessage,
	Whisper,
	UserEvent,
	Kick
};

enum class UserConnection
{
	Left,
	Joined
};

