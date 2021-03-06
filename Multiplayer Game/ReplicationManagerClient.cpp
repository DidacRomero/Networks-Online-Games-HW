#include "Networks.h"
#include "ReplicationManagerClient.h"

// TODO(you): World state replication lab session

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	//Check if the packet is empty
	if (packet.GetSize() > sizeof(PacketHeader))
	{
		uint16 packet_bytes = sizeof(PacketHeader) + sizeof(uint32) * 2;	//@ch0m5: The added 2 uint32 are from the packetSequenceNumber and the inputSequenceNumber

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

				//If the gameObject doesn't exist, create it else flag for destruction of the dummy
				if (App->modLinkingContext->isNetworkIndexOccupied(networkId) == false)
					App->modLinkingContext->registerNetworkGameObjectWithNetworkId(go, networkId);
				else
				{
					//Only called when a network gameobject should be destroyed, so it will always contain a GameObject *
					GameObject* to_destroy = App->modLinkingContext->getNetworkGameObject(networkId, false);
					App->modLinkingContext->unregisterNetworkGameObject(to_destroy);
					App->modGameObject->Destroy(to_destroy);

					//Register the new gameObject
					App->modLinkingContext->registerNetworkGameObjectWithNetworkId(go, networkId);
				}

				//Now we need to fill ALL the data of the gameObject, position, behaviour, sprite etc.
				packet >> go->position.x;
				packet >> go->position.y;
				packet >> go->angle;
				packet_bytes += sizeof(go->position.x);
				packet_bytes += sizeof(go->position.y);
				packet_bytes += sizeof(go->angle);

				//Receive sprite data
				std::string sprite_filename;
				packet >> sprite_filename;
				readSprite(sprite_filename,go, packet, packet_bytes);

				packet_bytes += sizeof(sprite_filename);
				packet_bytes += sizeof(go->sprite->order);

				//receive behaviour and collider info
				BehaviourType behaviour;
				packet >> behaviour;
				createBehaviour(behaviour, go);
				packet_bytes += sizeof(behaviour);
				packet >> go->tag;
				packet_bytes += sizeof(go->tag);

				// Interpolation
				if (go->networkId == App->modNetClient->getNetworkId())	// If object created is from this client, don't apply interpolation	// WARNING: This could not be working as intended
					go->networkInterpolationEnabled = false;
				else
					go->interpolation.prevPosition = go->position;
			}
			else if (action == ReplicateAction::UPDATE)
			{
				//If Update, get object from linking, deserialize
				GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);

				go->interpolation.prevPosition = go->position;
				go->interpolation.prevAngle = go->angle;

				//Check for Nulls & possibility of receiving an Update before a Create!!!!!!!
				if (go != nullptr)
				{
					packet >> go->position.x;
					packet >> go->position.y;
					packet >> go->angle;

					//If it's a spaceship update
					if (go->behaviour != nullptr && go->behaviour->type() == BehaviourType::Spaceship)
					{
						Spaceship* ship = (Spaceship*)go->behaviour;
						packet >> ship->hitPoints;
						packet_bytes += sizeof(ship->hitPoints);
					}

					// Interpolation
					if (App->modNetClient->isInterpolationEnabled() && go->networkInterpolationEnabled)
					{
						go->interpolation.lerpMaxTime = App->modNetClient->calcAvgReplicationTime();
						go->interpolation.secondsElapsed = 0.0f;

						go->interpolation.initialPosition = go->interpolation.prevPosition;
						go->interpolation.finalPosition = go->position;
						go->position = go->interpolation.initialPosition;

						go->interpolation.initialAngle = go->interpolation.prevAngle;
						go->interpolation.finalAngle = go->angle;
						go->angle = go->interpolation.initialAngle;
					}
				}

				packet_bytes += sizeof(go->position.x);
				packet_bytes += sizeof(go->position.y);
				packet_bytes += sizeof(go->angle);
			}
			else if (action == ReplicateAction::DESTROY)
			{
				//If destroy, get go from linking context, unregister & destroy
				GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId,false);
				if (go != nullptr)
				{
					DLOG(" Destroyed a GameObject from client");
					App->modLinkingContext->unregisterNetworkGameObject(go);
					App->modGameObject->Destroy(go);
				}
			}
		}
	}
}

void ReplicationManagerClient::readSprite(const std::string& filename, GameObject* go, const InputMemoryStream& packet, uint16 &packet_bytes)
{
	// Create sprite, give sprite ORDER & SIZE
	go->sprite = App->modRender->addSprite(go);
	packet >> go->sprite->order;
	packet_bytes += sizeof(go->sprite->order);

	float x, y;
	packet >> x;
	packet >> y;
	go->size = { x, y };

	//determine the texture
	if (strcmp(filename.c_str(), App->modResources->asteroid1->filename) == 0)	//ASTEROID 1
		go->sprite->texture = App->modResources->asteroid1;
	else if (strcmp(filename.c_str(), App->modResources->asteroid2->filename) == 0) //ASTEROID 2
		go->sprite->texture = App->modResources->asteroid2;
	else if (strcmp(filename.c_str(), App->modResources->spacecraft1->filename) == 0) //SPACECRAFT 1
		go->sprite->texture = App->modResources->spacecraft1;
	else if (strcmp(filename.c_str(), App->modResources->spacecraft2->filename) == 0) //SPACECRAFT 2
		go->sprite->texture = App->modResources->spacecraft2;
	else if (strcmp(filename.c_str(), App->modResources->spacecraft3->filename) == 0) //SPACECRAFT 3
		go->sprite->texture = App->modResources->spacecraft3;
	else if (strcmp(filename.c_str(), App->modResources->laser->filename) == 0) //LASER
		go->sprite->texture = App->modResources->laser;
	else if (strcmp(filename.c_str(), App->modResources->explosion1->filename) == 0) //EXPLOSION 1		(IT'S a SPECIAL CASE)
	{
		//Give sprite AND animation clip
		go->sprite->texture = App->modResources->explosion1;

		if (go->animation == nullptr)
			go->animation = App->modRender->addAnimation(go);

		go->animation->clip = App->modResources->explosionClip;
		App->modSound->playAudioClip(App->modResources->audioClipExplosion);	//Play the explosion audio clip
	}
}

//This Function assigns the correct behaviour
void ReplicationManagerClient::createBehaviour(BehaviourType behaviour, GameObject* go)
{
	Behaviour* b = App->modBehaviour->addBehaviour(behaviour, go);
	if (behaviour == BehaviourType::Spaceship)
	{
		go->behaviour = (Spaceship*)b;
		go->behaviour->isServer = false;
		go->behaviour->isLocalPlayer = false;
		go->collider = App->modCollision->addCollider(ColliderType::Player, go);
	}
	else if (behaviour == BehaviourType::Laser)
	{
		go->behaviour = (Laser*)b;
		go->behaviour->isServer = false;
		go->collider = App->modCollision->addCollider(ColliderType::Laser, go);
	}
}


