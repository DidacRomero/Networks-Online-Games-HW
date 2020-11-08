#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

// By Dídac Romero & Carles Homs

enum class ClientMessage
{
	Hello,
	ChatMessage,
	AnonChatMessage,
	Whisper,
	ChangeName,
	Mute,
	UnMute,
	MuteList,
	Kick,
	Ban,
	UnBan,
	BanList,
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
	MuteList,
	Kick,
	Ban,
	UnBan,
	BanList,
	List
};

enum class UserConnection
{
	Left,
	Joined,
	Aborted
};

