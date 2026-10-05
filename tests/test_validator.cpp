#include "SensorValidator.hpp"
#include <gtest/gtest.h>


namespace agri {

class SensorValidatorTest : public ::testing::Test {
protected:
  SensorValidator validator;
};

TEST_F(SensorValidatorTest, ValidDataPasses) {
  SensorData data;
  data.temperature = 25.0;
  data.humidity = 50.0;
  data.soilMoisture = 60.0;
  data.ph = 7.0;
  data.lightIntensity = 80.0;

  EXPECT_TRUE(validator.validate(data));
}

TEST_F(SensorValidatorTest, ExtremeTemperatureFails) {
  SensorData data;
  data.temperature =
      100.0; // Assume > 60 or 70 is invalid based on validator internals
  data.humidity = 50.0;
  data.soilMoisture = 60.0;
  data.ph = 7.0;
  data.lightIntensity = 80.0;

  EXPECT_FALSE(validator.validate(data));

  data.temperature = -50.0; // Assuming < -20 or 0 is invalid
  EXPECT_FALSE(validator.validate(data));
}

TEST_F(SensorValidatorTest, NegativeHumidityFails) {
  SensorData data;
  data.temperature = 25.0;
  data.humidity = -10.0;
  data.soilMoisture = 60.0;
  data.ph = 7.0;
  data.lightIntensity = 80.0;

  EXPECT_FALSE(validator.validate(data));
}

TEST_F(SensorValidatorTest, GreaterThan100PercentagesFails) {
  SensorData data;
  data.temperature = 25.0;
  data.humidity = 105.0;
  data.soilMoisture = 60.0;
  data.ph = 7.0;
  data.lightIntensity = 80.0;

  EXPECT_FALSE(validator.validate(data));
}

} // namespace agri
