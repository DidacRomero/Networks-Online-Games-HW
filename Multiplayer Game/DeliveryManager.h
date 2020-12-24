#pragma once

#include <queue>

// TODO(you): Reliability on top of UDP lab session

class DeliveryManager;

class DeliveryDelegate {
public:
    virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
    virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};

struct Delivery {
    Delivery(uint32 sequenceNumber);
    ~Delivery();

    uint32 sequenceNumber = 0;
    double dispatchTime = 0.0;
    DeliveryDelegate* delegate = nullptr;
};

// Sent along each packet in order to confirm its delivery
class DeliveryManager
{
public:
    // Constructor/Destructor
    DeliveryManager();
    ~DeliveryManager();

    // REDUNDANCY
    // Sender: Write new sequence numbers into a packet
    Delivery* writeSequenceNumber(OutputMemoryStream& packet);

    // Reciever: Process the sequence number from an incoming packet
    bool processSequenceNumber(const InputMemoryStream& packet);

    // ACKNOWLEDGEMENT
    // Receiver: Write ack'ed sequence numbers into a packet
    bool hasSequenceNumbersPendingAck();
    void writeSequenceNumbersPendingAck(OutputMemoryStream& packet);

    // Sender: Process ack'ed sequence numbers from a packet
    void processAckdSequenceNumbers(const InputMemoryStream& packet);
    void processTimedOutPackets();

    // Clear all queues and reset counters
    void clear();

private:
    // Sender
    uint32 nextOutGoingSequenceNumber = 0;          // Redundancy
    std::queue<Delivery*> pendingDeliveries;        // Acknowledgement

    // Receiver
    uint32 nextExpectedSequenceNumber = 0;          // Redundancy
    std::queue<uint32> sequenceNumbersPendingAck;   // Acknowledgement
};