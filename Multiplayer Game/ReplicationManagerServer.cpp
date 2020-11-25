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
	packet << ServerMessage::Replication;
	//Iterate the map to create a packet with all the changes
	for (std::unordered_map<uint32, ReplicationCommand>::iterator it = umap.begin(); it != umap.end(); ++it)
	{
	//Write the NetworkID
		packet << (*it).second.networkId;
	//Write the ReplicationAction
		packet << (*it).second.action;
	
		if ((*it).second.action == ReplicateAction::CREATE)
		{
			//If action Create
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it).second.networkId);
			packet << go->position.x;
			packet << go->position.y;
			packet << go->angle;
		}
		else if ((*it).second.action == ReplicateAction::UPDATE)	//It may seem code duplication for now, but in the future we might have to different things on update, sending different info
		{
			//If action Update
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it).second.networkId);
			packet << go->position.x;
			packet << go->position.y;
			packet << go->angle;
		}
		else if ((*it).second.action == ReplicateAction::DESTROY)
		{
			//If action Destroy (we don't really do anything here, but the else if is here for possible scalibilty in the future, 
			//in case we have other replication actions)
		}
	}
	//Clear remove the application command
	umap.clear();
}
