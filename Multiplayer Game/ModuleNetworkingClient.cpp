#include "ModuleNetworkingClient.h"


//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
}

float ModuleNetworkingClient::calcAvgReplicationTime() const
{
	float avg = 0.0f;
	for (int i = 0; i < REPLICATION_TIME_BUFFER_SIZE; ++i)
		avg += replicationTimeBuffer[i];

	return avg / REPLICATION_TIME_BUFFER_SIZE;
}

//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Connecting;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastHello = 9999.0f;
	secondsSinceLastInputDelivery = 0.0f;
	secSinceLastPing = 0.0f;
	secSinceLastPacket = 0.0f;
}

void ModuleNetworkingClient::onGui()
{
	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::Connecting)
		{
			ImGui::Text("Connecting to server...");
		}
		else if (state == ClientState::Connected)
		{
			ImGui::Text("Connected to server");

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Spaceship info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
			}
			else
			{
				//isnull, just to debug 
				int test = 0;
			}

			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);

			ImGui::Checkbox("Client Prediction", &clientPrediction);

			ImGui::Checkbox("Entity Interpolation", &mustEnableInterpolation);
		}
	}
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	// TODO(you): UDP virtual connection lab session	(DONE)
	secSinceLastPacket = 0.0f;

	uint32 protoId;
	packet >> protoId;
	if (protoId != PROTOCOL_ID) return;

	ServerMessage message;
	packet >> message;

	if (state == ClientState::Connecting)
	{
		if (message == ServerMessage::Welcome)
		{
			packet >> playerId;
			packet >> networkId;

			LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
			state = ClientState::Connected;
		}
		else if (message == ServerMessage::Unwelcome)
		{
			WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
			disconnect();
		}
	}
	else if (state == ClientState::Connected)
	{
		// @ch0m5 Custom Code
		//if (message == ServerMessage::Ping)
		//	LOG("Ping received from server!");	//IMPROVE: Add ms between Ping sent and Ping received?

		// TODO(you): World state replication lab session
		if (message == ServerMessage::Replication)
		{
			// TODO(you): Reliability on top of UDP lab session
			//@ch0m5: We read the inputId of the packet received
			if (delivery_manager_client.processSequenceNumber(packet))
			{
				uint32 nextInputSequenceNumber;
				packet >> nextInputSequenceNumber;
				inputDataFront = nextInputSequenceNumber;

				// We use a double boolean to restrict and control the de/activation of interpolation
				entityInterpolation = mustEnableInterpolation;

				//@didac: If we have to replicate read!
				replication_manager_client.read(packet);

				// CLIENT PREDICTION: Server Reconciliation
				if (clientPrediction)
				{
					GameObject* playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);

					for (uint32 i = inputDataFront; i < inputDataBack; ++i)
					{
						InputPacketData& inputPacketData = inputData[i % ArrayCount(inputData)];
						InputController controller = inputControllerFromInputPacketData(inputPacketData, controller);	// WARNING: Passing as a parameter at the same time as declaration, optimal but beware.

						if (playerGameObject != nullptr)
							playerGameObject->behaviour->onInput(controller);
					}
				}
			}

			// INTERPOLATION
			if (entityInterpolation)   // On recieved replication packet: Advance iteratior by 1 and reset respctive array value to 0
				replicationTimeBuffer[++replicationTimeIterator % ArrayCount(replicationTimeBuffer)] = 0.0f;

			// ACKNOWLEDGEMENT
			if (delivery_manager_client.hasSequenceNumbersPendingAck()) {
				OutputMemoryStream packet;
				packet << PROTOCOL_ID;
				packet << ClientMessage::Acknowledgement;
				delivery_manager_client.writeSequenceNumbersPendingAck(packet);
				sendPacket(packet, fromAddress);
			}
		}
	}
}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;

	// TODO(you): UDP virtual connection lab session	(DONE)
	secSinceLastPacket += Time.deltaTime;

	if (secSinceLastPacket > DISCONNECT_TIMEOUT_SECONDS)
	{
		LOG("Disconnected from server! Reason: High ping.");
		onDisconnect();
	}

	if (state == ClientState::Connecting)
	{
		secondsSinceLastHello += Time.deltaTime;

		if (secondsSinceLastHello > 0.1f)
		{
			secondsSinceLastHello = 0.0f;

			OutputMemoryStream packet;
			packet << PROTOCOL_ID;
			packet << ClientMessage::Hello;
			packet << playerName;
			packet << spaceshipType;

			sendPacket(packet, serverAddress);
		}
	}
	else if (state == ClientState::Connected)
	{
		// TODO(you): UDP virtual connection lab session	(DONE)
		secSinceLastPing += Time.deltaTime;

		if (secSinceLastPing > PING_INTERVAL_SECONDS)
		{
			secSinceLastPing = 0.0f;

			OutputMemoryStream packet;
			packet << PROTOCOL_ID;
			packet << ClientMessage::Ping;
			sendPacket(packet, serverAddress);

			//LOG("Ping sent to server!");
		}

		// Process more inputs if there's space
		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			// Pack current input
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);

			// CLIENT PREDICTION: Locally processed inputs (until server replication to resynch)
			if (clientPrediction)
			{
				GameObject* playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);

				if (playerGameObject != nullptr)
					playerGameObject->behaviour->onInput(Input);
			}

			secondsSinceLastInputDelivery += Time.deltaTime;

			// Input delivery interval timed out: create a new input packet
			if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
			{
				secondsSinceLastInputDelivery = 0.0f;

				OutputMemoryStream packet;
				packet << PROTOCOL_ID;
				packet << ClientMessage::Input;

				// TODO(you): Reliability on top of UDP lab session

				for (uint32 i = inputDataFront; i < inputDataBack; ++i)
				{
					InputPacketData& inputPacketData = inputData[i % ArrayCount(inputData)];
					packet << inputPacketData.sequenceNumber;
					packet << inputPacketData.horizontalAxis;
					packet << inputPacketData.verticalAxis;
					packet << inputPacketData.buttonBits;
				}

				// Clear the queue	//@ch0m5: Removed queue clear for Redundancy, the server now establishes the new Front
				//inputDataFront = inputDataBack;

				sendPacket(packet, serverAddress);
			}
		}

		// TODO(you): Latency management lab session
		// INTERPOLATION
		if (entityInterpolation)   // Add deltaTime to current buffer index each frame
			replicationTimeBuffer[replicationTimeIterator % ArrayCount(replicationTimeBuffer)] += Time.deltaTime;

		// Update camera for player
		GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
		if (playerGameObject != nullptr)
		{
			App->modRender->cameraPosition = playerGameObject->position;
		}
		else
		{
			// This means that the player has been destroyed (e.g. killed)
		}
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	uint16 networkGameObjectsCount;
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}
