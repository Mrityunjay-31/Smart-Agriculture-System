#include "SensorParser.hpp"
#include <gtest/gtest.h>


namespace agri {

class SensorParserTest : public ::testing::Test {
protected:
  SensorParser parser;
};

TEST_F(SensorParserTest, ValidArduinoString) {
  std::string input =
      "Temp: 24.5C | Hum: 45.2% | Soil: 55.0% | pH: 6.8 | Light: 85%";
  auto result = parser.parse(input);

  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->temperature, 24.5);
  EXPECT_DOUBLE_EQ(result->humidity, 45.2);
  EXPECT_DOUBLE_EQ(result->soilMoisture, 55.0);
  EXPECT_DOUBLE_EQ(result->ph, 6.8);
  EXPECT_DOUBLE_EQ(result->lightIntensity, 85.0);
}

TEST_F(SensorParserTest, PartialDataReturnsNullopt) {
  std::string input = "Temp: 24.5C | Hum: 45.2% | Soil: 55.0%";
  auto result = parser.parse(input);
  EXPECT_FALSE(result.has_value());
}

TEST_F(SensorParserTest, MalformedDataReturnsNullopt) {
  std::string input =
      "Temp: XYZ | Hum: 45.2% | Soil: 55.0% | pH: 6.8 | Light: 85%";
  auto result = parser.parse(input);
  EXPECT_FALSE(result.has_value());
}

TEST_F(SensorParserTest, EmptyStringReturnsNullopt) {
  auto result = parser.parse("");
  EXPECT_FALSE(result.has_value());
}

} // namespace agri
