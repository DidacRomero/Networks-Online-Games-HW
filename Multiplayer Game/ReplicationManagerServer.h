#pragma once

#include <unordered_map>
// TODO(you): World state replication lab session

class ReplicationManagerServer
{

public:	//METHODS

	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);

	void write(OutputMemoryStream &packet);

	void writeSprite(OutputMemoryStream& packet, GameObject* go);
	void writeAnimation(OutputMemoryStream& packet, GameObject* go);


private:	//VARS
	std::unordered_map <uint32, ReplicationCommand> umap;
};