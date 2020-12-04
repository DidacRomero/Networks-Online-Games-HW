#pragma once

// TODO(you): Reliability on top of UDP lab session

class DeliveryManager;

// Sent along each packet in order to confirm its delivery
class DeliveryManager
{
public:
    // Constructor/Destructor
    DeliveryManager();
    ~DeliveryManager();

    // Write sequence number into a packet
    void writeSeqNum(OutputMemoryStream& packet);

    // Recieve and process sequence number from a packet
    bool readSeqNum(const InputMemoryStream& packet);

private:
    // Sender
    uint32 nextSentSeqNum = 0;

    // Receiver
    uint32 nextExpectedSeqNum = 0;
};