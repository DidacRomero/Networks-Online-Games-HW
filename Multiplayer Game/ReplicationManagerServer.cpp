#include "Networks.h"
#include "ReplicationManagerServer.h"

// TODO(you): World state replication lab session
//All code here written by us Carles & Dídac
void ReplicationManagerServer::create(uint32 networkId)
{
	//Add to the map  with create instruction
	ReplicationCommand comm(networkId,ReplicateAction::CREATE);
	umap.emplace(networkId,comm);
}

void ReplicationManagerServer::update(uint32 networkId)
{
	//Add to the map  with create instruction
	ReplicationCommand comm(networkId, ReplicateAction::UPDATE);
	umap.emplace(networkId, comm);
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	//Add to the map  with create instruction
	ReplicationCommand comm(networkId, ReplicateAction::DESTROY);
	umap.emplace(networkId, comm);
}

void ReplicationManagerServer::write(OutputMemoryStream& packet)
{
	//Write the NetworkID
	
	//Write the ReplicationAction

	//If action Create


	//If action Update


	//If action Destroy

	//Clear remove the application command?
}
