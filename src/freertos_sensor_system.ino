#include <Arduino.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// HARDWARE
// ============================================================

#define DHT_PIN 4
#define DHT_TYPE DHT22

#define ALARM_LED 2

// ============================================================
// OBJECTS
// ============================================================

DHT dht(
  DHT_PIN,
  DHT_TYPE
);

LiquidCrystal_I2C lcd(
  0x27,
  16,
  2
);

// ============================================================
// SENSOR DATA
// ============================================================

struct SensorData
{
  float temperature;
  float humidity;
  bool valid;
};

// ============================================================
// FREERTOS OBJECTS
// ============================================================

QueueHandle_t sensorQueue;

SemaphoreHandle_t lcdMutex;

// ============================================================
// THRESHOLDS
// ============================================================

const float TEMPERATURE_LIMIT = 30.0;
const float HUMIDITY_LIMIT = 70.0;

// ============================================================
// TASK HANDLES
// ============================================================

TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t alarmTaskHandle = NULL;

// ============================================================
// SENSOR TASK
// ============================================================

void sensorTask(void *parameter)
{
  SensorData data;

  while (true)
  {
    float humidity =
      dht.readHumidity();

    float temperature =
      dht.readTemperature();

    if (
      isnan(humidity) ||
      isnan(temperature)
    )
    {
      data.valid = false;

      Serial.println(
        "[Sensor Task] Sensor read failed."
      );
    }
    else
    {
      data.temperature =
        temperature;

      data.humidity =
        humidity;

      data.valid = true;

      Serial.print(
        "[Sensor Task] Temperature: "
      );

      Serial.print(
        temperature,
        1
      );

      Serial.print(
        " C, Humidity: "
      );

      Serial.print(
        humidity,
        1
      );

      Serial.println(
        " %"
      );
    }

    // Send data to the queue
    if (
      xQueueSend(
        sensorQueue,
        &data,
        pdMS_TO_TICKS(100)
      ) != pdPASS
    )
    {
      Serial.println(
        "[Sensor Task] Queue is full."
      );
    }

    // Read sensor every 2 seconds
    vTaskDelay(
      pdMS_TO_TICKS(2000)
    );
  }
}

// ============================================================
// DISPLAY TASK
// ============================================================

void displayTask(void *parameter)
{
  SensorData data;

  while (true)
  {
    if (
      xQueueReceive(
        sensorQueue,
        &data,
        portMAX_DELAY
      ) == pdPASS
    )
    {
      if (!data.valid)
      {
        if (
          xSemaphoreTake(
            lcdMutex,
            pdMS_TO_TICKS(100)
          ) == pdTRUE
        )
        {
          lcd.clear();

          lcd.setCursor(0, 0);
          lcd.print("Sensor error");

          lcd.setCursor(0, 1);
          lcd.print("Check DHT22");

          xSemaphoreGive(
            lcdMutex
          );
        }

        continue;
      }

      if (
        xSemaphoreTake(
          lcdMutex,
          pdMS_TO_TICKS(100)
        ) == pdTRUE
      )
      {
        lcd.clear();

        lcd.setCursor(0, 0);

        lcd.print("T:");
        lcd.print(
          data.temperature,
          1
        );
        lcd.print(" C");

        lcd.setCursor(0, 1);

        lcd.print("H:");
        lcd.print(
          data.humidity,
          1
        );
        lcd.print(" %");

        xSemaphoreGive(
          lcdMutex
        );
      }
    }
  }
}

// ============================================================
// ALARM TASK
// ============================================================

void alarmTask(void *parameter)
{
  SensorData data;

  while (true)
  {
    /*
      The alarm task receives a copy of the
      latest sensor data from the queue.

      If the queue does not contain new data,
      the task waits briefly.
    */

    if (
      xQueueReceive(
        sensorQueue,
        &data,
        pdMS_TO_TICKS(500)
      ) == pdPASS
    )
    {
      if (!data.valid)
      {
        digitalWrite(
          ALARM_LED,
          LOW
        );

        continue;
      }

      bool alarm =
        data.temperature >
        TEMPERATURE_LIMIT ||
        data.humidity >
        HUMIDITY_LIMIT;

      if (alarm)
      {
        digitalWrite(
          ALARM_LED,
          HIGH
        );

        Serial.println(
          "[Alarm Task] WARNING: "
          "Threshold exceeded!"
        );
      }
      else
      {
        digitalWrite(
          ALARM_LED,
          LOW
        );
      }
    }

    vTaskDelay(
      pdMS_TO_TICKS(100)
    );
  }
}

// ============================================================
// SYSTEM MONITOR TASK
// ============================================================

void systemMonitorTask(void *parameter)
{
  while (true)
  {
    Serial.println(
      "[System] FreeRTOS is running."
    );

    Serial.print(
      "[System] Free heap: "
    );

    Serial.print(
      ESP.getFreeHeap()
    );

    Serial.println(
      " bytes"
    );

    Serial.print(
      "[System] CPU frequency: "
    );

    Serial.print(
      getCpuFrequencyMhz()
    );

    Serial.println(
      " MHz"
    );

    Serial.println(
      "------------------------------"
    );

    vTaskDelay(
      pdMS_TO_TICKS(5000)
    );
  }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println(
    "================================"
  );
  Serial.println(
    "ESP32 FreeRTOS Sensor System"
  );
  Serial.println(
    "================================"
  );

  // ----------------------------------------------------------
  // HARDWARE
  // ----------------------------------------------------------

  pinMode(
    ALARM_LED,
    OUTPUT
  );

  digitalWrite(
    ALARM_LED,
    LOW
  );

  dht.begin();

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("FreeRTOS System");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(1500);

  // ----------------------------------------------------------
  // CREATE QUEUE
  // ----------------------------------------------------------

  sensorQueue =
    xQueueCreate(
      10,
      sizeof(SensorData)
    );

  if (sensorQueue == NULL)
  {
    Serial.println(
      "ERROR: Queue creation failed!"
    );

    while (true)
    {
      delay(1000);
    }
  }

  // ----------------------------------------------------------
  // CREATE MUTEX
  // ----------------------------------------------------------

  lcdMutex =
    xSemaphoreCreateMutex();

  if (lcdMutex == NULL)
  {
    Serial.println(
      "ERROR: Mutex creation failed!"
    );

    while (true)
    {
      delay(1000);
    }
  }

  // ----------------------------------------------------------
  // CREATE TASKS
  // ----------------------------------------------------------

  xTaskCreate(
    sensorTask,
    "Sensor Task",
    4096,
    NULL,
    2,
    &sensorTaskHandle
  );

  xTaskCreate(
    displayTask,
    "Display Task",
    4096,
    NULL,
    1,
    &displayTaskHandle
  );

  xTaskCreate(
    alarmTask,
    "Alarm Task",
    4096,
    NULL,
    1,
    &alarmTaskHandle
  );

  xTaskCreate(
    systemMonitorTask,
    "System Monitor",
    4096,
    NULL,
    1,
    &alarmTaskHandle
  );

  Serial.println();
  Serial.println(
    "FreeRTOS tasks created."
  );

  Serial.println(
    "System started."
  );

  Serial.println();
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  /*
    The application logic is handled by FreeRTOS tasks.

    loop() is intentionally kept empty.
  */

  vTaskDelay(
    pdMS_TO_TICKS(1000)
  );
}
