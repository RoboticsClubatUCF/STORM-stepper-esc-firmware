typedef struct {
  SPI_HandleTypeDef *hspi1;
} SerialSPIParams_t;

void Trinamic_SPI_Task(void *pvParameters);