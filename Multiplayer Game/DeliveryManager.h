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

    // REDUNDANCY
    // Sender: Write sequence number into a packet
    void writeSequenceNumber(OutputMemoryStream& packet);

    // Reciever: Recieve and process sequence number from a packet
    bool readSequenceNumber(const InputMemoryStream& packet);

    // ACKNOWLEDGEMENT
    // Receiver: Check for pending Acks and write them into a packet
    bool hasPendingAcks();
    void writePendingAcks(OutputMemoryStream& packet);

    // Sender: Process acks from a packet
    void readAcks(const InputMemoryStream& packet);
    void readLostPackets();

    // Clear all queues and reset counters
    void clear();

private:
    // REDUNDANCY
    uint32 nextSentSequenceNumber = 0;          // Sender
    uint32 nextExpectedSequenceNumber = 0;      // Receiver

    // ACKNOWLEDGEMENT
    std::deque<Delivery*> pendingDeliveries;    // Sender
    std::deque<uint32> pendingAcks;             // Receiver
};