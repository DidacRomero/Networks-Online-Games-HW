#pragma once

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
	//Custom constructor
	ReplicationCommand(uint32 networkId, ReplicateAction action) : networkId(networkId), action(action) {}

	ReplicateAction action;
	uint32 networkId;
};
