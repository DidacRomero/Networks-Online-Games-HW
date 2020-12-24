#include "DeliveryManager.h"
#include "Networks.h"

// TODO(you): Reliability on top of UDP lab session

// DELIVERY
Delivery::Delivery(uint32 sequenceNumber)
    : sequenceNumber(sequenceNumber)
{
}

Delivery::~Delivery()
{}

// DELIVERY_MANAGER
DeliveryManager::DeliveryManager()
{
}

DeliveryManager::~DeliveryManager()
{
}

// REDUNDANCY
void DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
    uint32 sequenceNumber = nextSentSequenceNumber++;
    packet << sequenceNumber;

    return;
}

bool DeliveryManager::readSequenceNumber(const InputMemoryStream& packet)
{
    uint32 sequenceNumber;
    packet >> sequenceNumber;
    if (sequenceNumber >= nextExpectedSequenceNumber) {
        nextExpectedSequenceNumber = sequenceNumber + 1;
        return true;
    }

    return false;
}

// ACKNOWLEDGEMENT
bool DeliveryManager::hasPendingAcks()
{
	return !pendingAcks.empty();
}

void DeliveryManager::writePendingAcks(OutputMemoryStream& packet)
{
	std::size_t ackSize = pendingAcks.size();
	packet << ackSize;

	if (ackSize > 0)
	{
		packet << pendingAcks.front();

		// Clear Queue
		std::queue<uint32> emptyQueue;
		std::swap(pendingAcks, emptyQueue);
	}
}

void DeliveryManager::readAcks(const InputMemoryStream& packet)
{
	std::size_t ackSize;
	packet >> ackSize;

	if (ackSize > 0)
	{
		uint32 firstAckNum;
		packet >> firstAckNum;

		uint32 nextAckNum = firstAckNum;
		uint32 onePastAckdSequenceNumber = nextAckNum + (uint32)ackSize;	// WARNING: Carles check if the cast works correctly

		while (nextAckNum < onePastAckdSequenceNumber && !pendingDeliveries.empty())
		{
			Delivery* delivery = pendingDeliveries.front();
			if (delivery->sequenceNumber == nextAckNum)
			{
				// CARLES: Check

				RELEASE(delivery);
				pendingDeliveries.pop();
				++nextAckNum;
			}
			else if (delivery->sequenceNumber < nextAckNum)
			{
				Delivery* deliveryCopy = delivery;
				pendingDeliveries.pop();
				
				// CARLES: Check

				RELEASE(delivery);
			}
			else
				++nextAckNum;
		}
	}
}

void DeliveryManager::readLostPackets()
{
	while (!pendingDeliveries.empty())
	{
		Delivery* delivery = pendingDeliveries.front();
		if (Time.time - delivery->timestamp >= ACK_INTERVAL_SECONDS)
		{
			// CARLES: Check

			RELEASE(delivery);
			pendingDeliveries.pop();
		}
		else
			break;
	}
}

void DeliveryManager::clear()
{
	while (!pendingDeliveries.empty())
	{
		RELEASE(pendingDeliveries.front());
		pendingDeliveries.pop();
	}

	// Clear Queues
	std::queue<Delivery*> emptyDeliveryQueue;
	std::swap(pendingDeliveries, emptyDeliveryQueue);

	std::queue<uint32> emptyNumQueue;
	std::swap(pendingAcks, emptyNumQueue);

	// Reset Counters
	nextSentSequenceNumber = nextExpectedSequenceNumber = 0;
}