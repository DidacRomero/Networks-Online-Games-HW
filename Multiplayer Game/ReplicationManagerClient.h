#pragma once

enum class SpriteType;
enum class BehaviourType : uint8;
// TODO(you): World state replication lab session
class ReplicationManagerClient
{

public: //METHODS

	void read(const InputMemoryStream &packet);
	void readSprite(SpriteType s_type, GameObject* go);
	void createBehaviour(BehaviourType behaviour, GameObject* go);
};