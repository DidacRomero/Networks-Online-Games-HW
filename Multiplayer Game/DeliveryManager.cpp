#include "DeliveryManager.h"
#include "Networks.h"

// TODO(you): Reliability on top of UDP lab session

// DELIVERY_MANAGER
DeliveryManager::DeliveryManager()
{
}

DeliveryManager::~DeliveryManager()
{
}

void DeliveryManager::writeSeqNum(OutputMemoryStream& packet)
{
    uint32 sequenceNumber = nextSentSeqNum++;
    packet << sequenceNumber;

    return;
}

bool DeliveryManager::readSeqNum(const InputMemoryStream& packet)
{
    uint32 sequenceNumber;
    packet >> sequenceNumber;
    if (sequenceNumber >= nextExpectedSeqNum) {
        nextExpectedSeqNum = sequenceNumber + 1;
        return true;
    }

    return false;
}