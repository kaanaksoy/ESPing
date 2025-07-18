

enum OperationState {
    INITIALIZING,
    CONNECTING,
    BUTTON_ACTION,
    GET_UPDATES,
    PUBLISH_UPDATES
}

struct State {
    OperationState operationState;
    bool wifiConnected = false;
    bool mqttConnected = false;
    bool mqttSubscribed = false;
    bool availabilityPublished = false;
    bool discoverySent = false;
    bool buttonPressedMessagePending = false;
    bool buttonReleasedMessagePending = false;
}
