#include "Networks.h"

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
				packet >> go->position.x;
				packet >> go->position.y;
				packet >> go->angle;

				packet_bytes += sizeof(go->position.x);
				packet_bytes += sizeof(go->position.y);
				packet_bytes += sizeof(go->angle);
			}
			else if (action == ReplicateAction::UPDATE)
			{
				//If Update, get object from linking, deserialize
				GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);

				packet >> go->position.x;
				packet >> go->position.y;
				packet >> go->angle;

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
