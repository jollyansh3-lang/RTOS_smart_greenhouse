/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os2.h" // CMSIS-RTOS v2 API

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Structure to hold all environmental variables passed through the queue
typedef struct {
    float temperature;
    float humidity;
    uint32_t moisture;
} GreenhouseData_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MOISTURE_THRESHOLD 300 // Custom threshold for dry soil
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hi2c1;
ADC_HandleTypeDef huart1;

/* Definitions for Tasks, Queues, and Mutexes */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4, // 1024 bytes
  .priority = (osPriority_t) osPriorityLow,
};

osThreadId_t ControlTaskHandle;
const osThreadAttr_t ControlTask_attributes = {
  .name = "ControlTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osMessageQueueId_t SensorDataQueueHandle;
const osMessageQueueAttr_t SensorDataQueue_attributes = {
  .name = "SensorDataQueue"
};

osMutexId_t DisplayMutexHandle;
const osMutexAttr_t DisplayMutex_attributes = {
  .name = "DisplayMutex"
};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

void StartSensorTask(void *argument);
void StartControlTask(void *argument);

/* USER CODE BEGIN PFP */
// Mock sensor reading functions - replace with library specific calls
float DHT11_ReadTemperature(void);
float DHT11_ReadHumidity(void);
void SecurePrintf(const char* format, float val1, float val2, uint32_t val3);
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* Initialize Remote Scheduler Engine */
  osKernelInitialize();

  /* Create the Mutex for safe shared resource access */
  DisplayMutexHandle = osMutexNew(&DisplayMutex_attributes);

  /* Create the Queue (Capacity: 5 elements of GreenhouseData_t size) */
  SensorDataQueueHandle = osMessageQueueNew(5, sizeof(GreenhouseData_t), &SensorDataQueue_attributes);

  /* Create the Tasks/Threads */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);
  ControlTaskHandle = osThreadNew(StartControlTask, NULL, &ControlTask_attributes);

  /* Start scheduler */
  osKernelStart();

  /* We should never reach here as control is given to the scheduler */
  while (1)
  {
  }
}

/* USER CODE BEGIN USER_CODE_REFAL */

/**
  * @brief  Task 1: Periodically reads sensors and places data onto a queue.
  * @param  argument: Not used
  */
void StartSensorTask(void *argument)
{
  GreenhouseData_t sensorLog;

  for(;;)
  {
    // 1. Fetch Ambient Measurements
    sensorLog.temperature = DHT11_ReadTemperature();
    sensorLog.humidity = DHT11_ReadHumidity();

    // 2. Fetch Analog Soil Moisture
    HAL_ADC_Start(&hadc1);
    if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      sensorLog.moisture = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    // 3. Thread-Safe Debugging Output via Mutex
    if (osMutexAcquire(DisplayMutexHandle, osWaitForever) == osOK)
    {
      printf("[Sensor Task] Data sampled. Sending to queue...\r\n");
      osMutexRelease(DisplayMutexHandle);
    }

    // 4. Send structured package to Queue (Wait up to 10 ticks if queue is full)
    osMessageQueuePut(SensorDataQueueHandle, &sensorLog, 0, 10);

    // Block this execution line for 2000ms to let lower priority tasks run
    osDelay(2000);
  }
}

/**
  * @brief  Task 2: Extracts sensor data packages from queue and executes control loops.
  * @param  argument: Not used
  */
void StartControlTask(void *argument)
{
  GreenhouseData_t operationalData;

  for(;;)
  {
    // Block indefinitely until a data parcel arrives in the queue
    if (osMessageQueueGet(SensorDataQueueHandle, &operationalData, NULL, osWaitForever) == osOK)
    {
      // Core Actuator Decision Loop
      if (operationalData.moisture < MOISTURE_THRESHOLD)
      {
        // Soil is dry: Activate relay/water pump connected to GPIO Pin 1
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        SecurePrintf("[Control Task] Pump status: ON | Temp: %.1fC | Moisture: %ld\r\n",
                     operationalData.temperature, 0.0, operationalData.moisture);
      }
      else
      {
        // Soil is saturated: Deactivate relay/water pump
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        SecurePrintf("[Control Task] Pump status: OFF | Temp: %.1fC | Moisture: %ld\r\n",
                     operationalData.temperature, 0.0, operationalData.moisture);
      }
    }
  }
}

/**
  * @brief  Helper utility ensuring console transmissions don't collide
  */
void SecurePrintf(const char* format, float val1, float val2, uint32_t val3)
{
  if (osMutexAcquire(DisplayMutexHandle, osWaitForever) == osOK)
  {
    printf(format, val1, val3); // Handles simplified formatted output
    osMutexRelease(DisplayMutexHandle);
  }
}

/* Dummy implementations representing sensor API hardware bindings */
float DHT11_ReadTemperature(void) { return 24.5f; }
float DHT11_ReadHumidity(void)    { return 60.0f; }

/* USER CODE END USER_CODE_REFAL */

// (Standard CubeMX clock and hardware peripheral initialization functions continue below...)
void SystemClock_Config(void) {}
static void MX_GPIO_Init(void) {
  // Configures GPIOA Pin 1 as standard digital output push-pull
}
static void MX_ADC1_Init(void) {}
static void MX_I2C1_Init(void) {}
static void MX_USART1_UART_Init(void) {}
