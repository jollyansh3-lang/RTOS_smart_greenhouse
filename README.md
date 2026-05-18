# RTOS_smart_greenhouse
In traditional farming, manual monitoring of environmental factors like soil moisture and temperature is inefficient and prone to human error. A Smart Greenhouse requires a system that can simultaneously monitor multiple sensors, control irrigation logic in real-time, and ensure system safety (e.g., preventing water overflow). An RTOS is necessary here to manage these concurrent tasks, ensuring that a slow sensor-reading task does not delay a critical water-cutoff command.
```mermaid
graph TD
    %% Define Styles and Colors for the Architecture
    classDef input fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    classDef mcu fill:#fff3e0,stroke:#f57c00,stroke-width:2px;
    classDef kernel fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px;
    classDef output fill:#e8f5e9,stroke:#388e3c,stroke-width:2px;

    %% Hardware Inputs Section
    subgraph Inputs [Sensors & Inputs]
        A[DHT11 Sensor<br>Temp & Humidity]:::input
        B[FC-28 Sensor<br>Analog Soil Moisture]:::input
    end

    %% Processing Unit (STM32 Processing Framework)
    subgraph MCU [STM32F103C8T6 Blue Pill Microcontroller]
        subgraph FreeRTOS [FreeRTOS Kernel Environment]
            T1[SensorTask<br>Priority: Low]:::kernel
            T2[ControlTask<br>Priority: Normal]:::kernel
            Q1((SensorDataQueue<br>Queue Buffer)):::kernel
            M1((DisplayMutex<br>Resource Lock)):::kernel
        end
    end

    %% Hardware Outputs Section
    subgraph Outputs [Actuators & Displays]
        C[5V Relay Module<br>Water Pump]:::output
        D[SSD1306 OLED<br>Data Display]:::output
        E[PC Terminal<br>UART Serial Debug]:::output
    end

    %% Hardware Interface Interconnections
    A -->|GPIO 1-Wire Protocol| T1
    B -->|Internal ADC1 Channel| T1
    
    %% Internal RTOS Data Flow Routing
    T1 -->|osMessageQueuePut| Q1
    Q1 -->|osMessageQueueGet| T2
    
    %% Actuator Outputs & Shared Resource Management
    T2 -->|GPIO Digital Output| C
    T2 -.->|Mutex Protected Access| M1
    D -.->|I2C Bus Access via| M1
    E -.->|UART Tx Line via| M1
