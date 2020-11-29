#include "Networks.h"
#include "ReplicationManagerClient.h"

// TODO(you): World state replication lab session

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	//Check if the packet is empty
	if (packet.GetSize() > sizeof(PacketHeader))
	{
		uint16 packet_bytes = sizeof(PacketHeader);
		//Iterate as long as we haven't finished emptying the packet
		while (packet.GetSize() > packet_bytes)
		{
			//read networkId
			uint32 networkId;
			packet >> networkId;
			packet_bytes += sizeof(networkId);

			//Read Application Action
			ReplicateAction action;
			packet >> action;
			packet_bytes += sizeof(action);

			if (action == ReplicateAction::CREATE)
			{
				//If Create, create the gameobject, link it & deserialize
				GameObject* go = App->modGameObject->Instantiate();
				go->networkId = networkId;
				App->modLinkingContext->registerNetworkGameObject(go);

				//Now we need to fill ALL the data of the gameObject, position, behaviour, sprite etc.
				packet >> go->position.x;
				packet >> go->position.y;
				packet >> go->angle;
				go->size = { 100,100 }; //HARDCODE REMOVE DIDAC

				BehaviourType behaviour;
				packet >> behaviour;
				createBehaviour(behaviour,go);
				packet >> go->tag;


				//Receive sprite data
				SpriteType s_type;
				packet >> s_type;
				readSprite(s_type,go);

				packet_bytes += sizeof(go->position.x);
				packet_bytes += sizeof(go->position.y);
				packet_bytes += sizeof(go->angle);
				packet_bytes += sizeof(behaviour);
				packet_bytes += sizeof(go->tag);
				packet_bytes += sizeof(s_type);
			}
			else if (action == ReplicateAction::UPDATE)
			{
				//If Update, get object from linking, deserialize
				GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);

				//Check for Nulls & possibility of receiving an Update before a Create!!!!!!!
				if (go != nullptr)
				{
					packet >> go->position.x;
					packet >> go->position.y;
					packet >> go->angle;
				}

				packet_bytes += sizeof(go->position.x);
				packet_bytes += sizeof(go->position.y);
				packet_bytes += sizeof(go->angle);
			}
			else if (action == ReplicateAction::DESTROY)
			{
				//If destroy, get go from linking context, unregister & destroy
				GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);
				App->modLinkingContext->unregisterNetworkGameObject(go);
				App->modGameObject->Destroy(go);
			}
		}
	}
}

void ReplicationManagerClient::readSprite(SpriteType s_type, GameObject* go)
{
	// Create sprite
	go->sprite = App->modRender->addSprite(go);
	go->sprite->order = 5;
	//First test with 1 texture
	go->sprite->texture = App->modResources->spacecraft1;

	//Later on, take into account type of spaceship
	/*if (spaceshipType == 0) {
		go->sprite->texture = App->modResources->spacecraft1;
	}
	else if (spaceshipType == 1) {
		go->sprite->texture = App->modResources->spacecraft2;
	}
	else {
		go->sprite->texture = App->modResources->spacecraft3;
	}*/
}

void ReplicationManagerClient::createBehaviour(BehaviourType behaviour, GameObject* go)
{
	Behaviour* b = App->modBehaviour->addBehaviour(behaviour, go);
	if (behaviour == BehaviourType::Spaceship)
	{
		go->behaviour = (Spaceship*)b;
		go->behaviour->isServer = true;
	}
	else if (behaviour == BehaviourType::Laser)
	{
		go->behaviour = (Laser*)b;
		go->behaviour->isServer = false;
	}
}


