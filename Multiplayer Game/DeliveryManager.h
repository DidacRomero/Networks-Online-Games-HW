#pragma once

// TODO(you): Reliability on top of UDP lab session

class DeliveryManager;

class DeliveryDelegate
{
public:
    virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
    virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};

struct Delivery
{
    Delivery(uint32 inputId);
    ~Delivery();

    uint32 inputId = 0;
};

// Sent along each packet in order to confirm its delivery
class DeliveryManager
{
public:
    // Constructor/Destructor
    DeliveryManager();
    ~DeliveryManager();

    // Write input ID into a packet
    Delivery* writeInputId(OutputMemoryStream& packet);

    // Recieve input ID from a packet
    bool readInputId(const InputMemoryStream& packet);

private:
    // Sender
    uint32 nextSentInputId = 0;
    std::deque<Delivery*> deliveriesToSend;

    // Receiver
    uint32 nextExpectedInputId = 0;
};
