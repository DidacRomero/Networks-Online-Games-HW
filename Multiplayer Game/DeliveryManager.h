#pragma once

// TODO(you): Reliability on top of UDP lab session

class DeliveryManager;

struct Delivery {
    Delivery(uint32 sequenceNumber);
    ~Delivery();

    uint32 sequenceNumber = 0;
    double timestamp = 0.0;
};

// Sent along each packet in order to confirm its delivery
class DeliveryManager
{
public:
    // Constructor/Destructor
    DeliveryManager();
    ~DeliveryManager();

    // Write sequence number into a packet
    void writeSequenceNumber(OutputMemoryStream& packet);

    // Recieve and process sequence number from a packet
    bool readSequenceNumber(const InputMemoryStream& packet);

private:
    // Sender
    uint32 nextSentSequenceNumber = 0;

    // Receiver
    uint32 nextExpectedSequenceNumber = 0;
};