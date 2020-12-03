#pragma once

// TODO(you): Reliability on top of UDP lab session

// Sent along each packet in order to confirm its delivery
class DeliveryManager
{
public:
    // Constructor/Destructor
    DeliveryManager();
    ~DeliveryManager();

    // Write input ID into a packet
    void writeInputId(OutputMemoryStream& packet);

    // Recieve input ID from a packet
    bool readInputId(const InputMemoryStream& packet);

private:
    // Sender
    uint32 nextSentSeqNum = 0;

    // Receiver
    uint32 nextExpectedSeqNum = 0;
};
