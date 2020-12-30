#pragma once

#include "ReplicationCommand.h"
#include <unordered_map>
// TODO(you): World state replication lab session

class ServerReplicationDelegate;

class ReplicationManagerServer
{

public:	//METHODS

	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);

	void write(OutputMemoryStream &packet, ServerReplicationDelegate* deliveryDelegate);

	void writeSprite(OutputMemoryStream& packet, GameObject* go);
	void writeAnimation(OutputMemoryStream& packet, GameObject* go);


private:	//VARS
	std::unordered_map <uint32, ReplicationCommand> umap;
};

class ServerReplicationDelegate : public DeliveryDelegate
{
public:
	ServerReplicationDelegate(ReplicationManagerServer* replicationManager);
	~ServerReplicationDelegate();

	void onDeliverySuccess(DeliveryManager* deliveryManager) override;
	void onDeliveryFailure(DeliveryManager* deliveryManager) override;

	void addCommand(const ReplicationCommand& replicationCommand);

private:
	ReplicationManagerServer* replicationManager = nullptr;
	std::vector<ReplicationCommand> replicationCommands;
};