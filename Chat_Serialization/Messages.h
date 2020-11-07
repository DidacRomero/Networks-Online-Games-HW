#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	ChatMessage,
	AnonChatMessage,
	Whisper,
	ChangeName,
	Mute,
	UnMute,
	Kick,
	List
};

enum class ServerMessage
{
	Welcome,
	NonWelcome,
	ChatMessage,
	AnonChatMessage,
	Whisper,
	ChangeName,
	UserEvent,
	Mute,
	UnMute,
	Kick,
	List
};

enum class UserConnection
{
	Left,
	Joined
};

