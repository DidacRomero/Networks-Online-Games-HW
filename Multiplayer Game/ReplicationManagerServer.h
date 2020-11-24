#pragma once

#include <unordered_map>
// TODO(you): World state replication lab session

enum class ReplicateAction
{
	NONE = 0,
	CREATE,
	UPDATE,
	DESTROY
};

struct ReplicationCommand
{
	ReplicateAction action;
	uint32 networkId;
};


class ReplicationManagerServer
{

public:	//METHODS

	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);

	void write(OutputMemoryStream &packet);

private:	//VARS
	std::unordered_map <uint32, ReplicationCommand> umap;
};