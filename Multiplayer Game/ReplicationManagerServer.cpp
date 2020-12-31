#include "Networks.h"
#include "ReplicationManagerServer.h"
#include "Maths.h"

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
	//Add to the map  with update instruction
	ReplicationCommand comm(networkId, ReplicateAction::UPDATE);
	umap.emplace(networkId, comm);
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	//Add to the map  with destroy instruction
	ReplicationCommand comm(networkId, ReplicateAction::DESTROY);
	umap.emplace(networkId, comm);
}

void ReplicationManagerServer::write(OutputMemoryStream& packet, ServerReplicationDelegate* deliveryDelegate)
{
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

			//Pass sprite, colliders & other data necessary for creation
			writeSprite(packet, go);

			//Pass behaviour info
			if(go->behaviour != nullptr)
			packet << go->behaviour->type();
			else
			packet << BehaviourType::None;

			//Pass collider info
			packet << go->tag;
		}
		else if ((*it).second.action == ReplicateAction::UPDATE)	//It may seem code duplication for now, but in the future we might have to different things on update, sending different info
		{
			//If action Update
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it).second.networkId);
			if (go != nullptr)
			{
				packet << go->position.x;
				packet << go->position.y;
				packet << go->angle;

				if(go->behaviour != nullptr && go->behaviour->type() == BehaviourType::Spaceship)
				{
					Spaceship* ship = (Spaceship*)go->behaviour;
					//Update life
					packet << ship->hitPoints;
				}
			}
		}
		else if ((*it).second.action == ReplicateAction::DESTROY)
		{
			//If action Destroy (we don't really do anything here, but the else if is here for possible scalibilty in the future, 
			//in case we have other replication actions)
		}

		// ACKNOWLEDGEMENT
		deliveryDelegate->addCommand((*it).second);
		(*it).second.action = ReplicateAction::NONE;	//WARNING: This could be a cause of problems
	}

	//Clear remove the application command
	umap.clear();
}

void ReplicationManagerServer::writeSprite(OutputMemoryStream& packet, GameObject* go)
{
	//Give us the filename
	std::string str = go->sprite->texture->filename;
	packet << str;
	packet << go->sprite->order;

	packet << go->size.x;
	packet << go->size.y;
}

void ReplicationManagerServer::writeAnimation(OutputMemoryStream& packet, GameObject* go)
{
	//packet << go->animation->clip
}

// SERVER REPLICATION DELEGATE
ServerReplicationDelegate::ServerReplicationDelegate(ReplicationManagerServer* replicationManager) : replicationManager(replicationManager)
{
}

ServerReplicationDelegate::~ServerReplicationDelegate()
{
}

void ServerReplicationDelegate::onDeliverySuccess(DeliveryManager* deliveryManager)
{
	return;	// Commend if we ever need to do something here, follow loop below

	for (const ReplicationCommand& replicationCommand : replicationCommands)
	{
		if (App->modLinkingContext->getNetworkGameObject(replicationCommand.networkId) != nullptr)
		{
			// In case we wanted to do something, do it here
		}
	}
}

void ServerReplicationDelegate::onDeliveryFailure(DeliveryManager* deliveryManager)
{
	for (const ReplicationCommand& replicationCommand : replicationCommands)
	{
		if (App->modLinkingContext->getNetworkGameObject(replicationCommand.networkId) != nullptr)
		{
			switch (replicationCommand.action)
			{
			case ReplicateAction::CREATE:
				replicationManager->create(replicationCommand.networkId);
				break;

			case ReplicateAction::UPDATE:
				replicationManager->update(replicationCommand.networkId);
				break;

			case ReplicateAction::DESTROY:
				replicationManager->destroy(replicationCommand.networkId);
				break;
			}
		}
	}
}

void ServerReplicationDelegate::addCommand(const ReplicationCommand& replicationCommand)
{
	replicationCommands.push_back(replicationCommand);
}
