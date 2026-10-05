#include "SensorProcessor.hpp"
#include "ThresholdConfig.hpp"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

namespace agri {

class SensorProcessorTest : public ::testing::Test {
protected:
  ThresholdConfig thresholds;
  const std::string confPath = "test_thresholds.conf";

  void SetUp() override {
    std::ofstream f(confPath);
    f << "temp_min=20.0\ntemp_max=30.0\n";
    f << "hum_min=40.0\nhum_max=60.0\n";
    f << "soil_min=30.0\nsoil_max=70.0\n";
    f << "ph_min=6.0\nph_max=7.5\n";
    f << "light_min=20.0\n";
    f.close();

    thresholds.load(confPath);
  }

  void TearDown() override { std::remove(confPath.c_str()); }
};

TEST_F(SensorProcessorTest, OptimalDataAssignsOptimalStatus) {
  SensorProcessor processor(thresholds);
  SensorData data;
  data.temperature = 25.0;
  data.humidity = 50.0;
  data.soilMoisture = 50.0;
  data.ph = 6.5;
  data.lightIntensity = 50.0;

  processor.process(data);

  EXPECT_EQ(data.overallStatus, "OPTIMAL");
  EXPECT_EQ(data.temperatureStatus, SensorStatus::OPTIMAL);
}

TEST_F(SensorProcessorTest, WarningDataAssignsWarningStatus) {
  SensorProcessor processor(thresholds);
  SensorData data;
  data.temperature =
      30.5; // Above max 30.0 but below critical (usually + 2.0 or 5.0)
  data.humidity = 50.0;
  data.soilMoisture = 50.0;
  data.ph = 6.5;
  data.lightIntensity = 50.0;

  processor.process(data);

  EXPECT_EQ(data.temperatureStatus, SensorStatus::WARNING);
  EXPECT_EQ(data.overallStatus, "WARNING");
}

} // namespace agri
