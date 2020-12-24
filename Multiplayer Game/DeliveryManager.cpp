#include "DeliveryManager.h"
#include "Networks.h"

// TODO(you): Reliability on top of UDP lab session

// DELIVERY
Delivery::Delivery(uint32 sequenceNumber)
    : sequenceNumber(sequenceNumber)
{
	dispatchTime = Time.time;
}

Delivery::~Delivery()
{
	RELEASE(deliveryDelegate);
}

// DELIVERY_MANAGER
DeliveryManager::DeliveryManager()
{
}

DeliveryManager::~DeliveryManager()
{
	clear();
}

// REDUNDANCY
Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
    uint32 sequenceNumber = nextOutGoingSequenceNumber++;
    packet << sequenceNumber;

	Delivery* delivery = new Delivery(sequenceNumber);
	pendingDeliveries.push(delivery);

    return delivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
    uint32 sequenceNumber;
    packet >> sequenceNumber;

    if (sequenceNumber >= nextExpectedSequenceNumber) {
		sequenceNumbersPendingAck.push(sequenceNumber);
        nextExpectedSequenceNumber = sequenceNumber + 1;
        return true;
    }

    return false;
}

// ACKNOWLEDGEMENT
bool DeliveryManager::hasSequenceNumbersPendingAck() const
{
	return !sequenceNumbersPendingAck.empty();
}

void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
	std::size_t ackSize = sequenceNumbersPendingAck.size();
	packet << ackSize;

	if (ackSize > 0)
	{
		packet << sequenceNumbersPendingAck.front();

		// Clear Queue
		std::queue<uint32> emptyQueue;
		std::swap(sequenceNumbersPendingAck, emptyQueue);
	}
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream& packet)	//TODO: Carles, implement delivery success/failure procedures
{
	std::size_t ackSize;
	packet >> ackSize;

	if (ackSize > 0)
	{
		uint32 firstAckNum;
		packet >> firstAckNum;

		uint32 nextAckNum = firstAckNum;
		uint32 onePastLast = nextAckNum + (uint32)ackSize;	// WARNING: Carles, check if the cast works correctly

		while (nextAckNum < onePastLast && !pendingDeliveries.empty())
		{
			Delivery* delivery = pendingDeliveries.front();
			if (delivery->sequenceNumber == nextAckNum)
			{
				//delivery->deliveryDelegate->onDeliverySuccess(this);

				RELEASE(delivery);
				pendingDeliveries.pop();

				++nextAckNum;
			}
			else if (delivery->sequenceNumber < nextAckNum)
			{
				//delivery->deliveryDelegate->onDeliveryFailure(this);	// WARNING: If something breaks it might be because of this

				RELEASE(delivery);
				pendingDeliveries.pop();
			}
			else
				++nextAckNum;
		}
	}
}

void DeliveryManager::processTimedOutPackets()
{
	while (!pendingDeliveries.empty())
	{
		Delivery* delivery = pendingDeliveries.front();
		if (Time.time - delivery->dispatchTime >= ACK_INTERVAL_SECONDS)
		{
			//delivery->deliveryDelegate->onDeliveryFailure(this);

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
	std::swap(sequenceNumbersPendingAck, emptyNumQueue);

	// Reset Counters
	nextOutGoingSequenceNumber = nextExpectedSequenceNumber = 0;
}