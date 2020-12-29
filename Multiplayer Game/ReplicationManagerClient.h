#pragma once

enum class SpriteType;
enum class BehaviourType : uint8;
// TODO(you): World state replication lab session
class ReplicationManagerClient
{

public: //METHODS

	void read(const InputMemoryStream &packet);
	void readSprite(const std::string&  filename, GameObject* go, const InputMemoryStream& packet, uint16& packet_bytes);
	void createBehaviour(BehaviourType behaviour, GameObject* go);
};