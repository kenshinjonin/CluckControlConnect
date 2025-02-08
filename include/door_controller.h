#pragma once

class DoorController {
public:
    static void init();
    static void open();
    static void close();
    static void stop();
    static void handleButtonPress();
    static void openDoorTask(void *parameter);
    static void closeDoorTask(void *parameter);
    static void buttonTask(void *parameter);
    
private:
    static bool isDoorMoving;
    static bool previousDoorDirectionUp;
    static unsigned long lastButtonPressTimestamp;
}; 