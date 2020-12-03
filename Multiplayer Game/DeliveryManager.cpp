#include "DeliveryManager.h"
#include "Networks.h"

// TODO(you): Reliability on top of UDP lab session

DeliveryManager::DeliveryManager()
{
}

DeliveryManager::~DeliveryManager()
{

}

void DeliveryManager::writeInputId(OutputMemoryStream& packet)
{
    uint32 inputId = nextSentSeqNum++;
    packet.Write(inputId);
}

bool DeliveryManager::readInputId(const InputMemoryStream& packet)
{
    uint32 inputId;
    packet.Read(inputId);
    if (inputId >= nextExpectedSeqNum) {
        nextExpectedSeqNum = inputId + 1;
        return true;
    }

    return false;
}