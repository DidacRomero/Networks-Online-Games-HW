#include "DeliveryManager.h"
#include "Networks.h"

// TODO(you): Reliability on top of UDP lab session

// DELIVERY

Delivery::Delivery(uint32 inputId)
    : inputId(inputId)
{
}

Delivery::~Delivery()
{
}

// DELIVERY_MANAGER
DeliveryManager::DeliveryManager()
{
}

DeliveryManager::~DeliveryManager()
{

}

Delivery* DeliveryManager::writeInputId(OutputMemoryStream& packet)
{
    uint32 inputId = nextSentInputId++;
    packet.Write(inputId);

    Delivery* delivery = new Delivery(inputId);
    deliveriesToSend.push_back(delivery);

    return delivery;
}

bool DeliveryManager::readInputId(const InputMemoryStream& packet)
{
    uint32 inputId;
    packet.Read(inputId);
    if (inputId >= nextExpectedInputId) {
        nextExpectedInputId = inputId + 1;
        return true;
    }

    return false;
}