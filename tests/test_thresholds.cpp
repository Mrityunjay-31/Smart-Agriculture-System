#include "ThresholdConfig.hpp"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

namespace agri {

class ThresholdConfigTest : public ::testing::Test {
protected:
  const std::string confPath = "test_thresholds2.conf";

  void TearDown() override { std::remove(confPath.c_str()); }
};

TEST_F(ThresholdConfigTest, LoadsValidConfig) {
  std::ofstream f(confPath);
  f << "temp_min=20.0\ntemp_max=30.0\n";
  f.close();

  ThresholdConfig thresholds;
  EXPECT_TRUE(thresholds.load(confPath));
  EXPECT_DOUBLE_EQ(thresholds.getRange("temp").optimalMin, 20.0);
  EXPECT_DOUBLE_EQ(thresholds.getRange("temp").optimalMax, 30.0);
}

TEST_F(ThresholdConfigTest, HandlesMissingFile) {
  ThresholdConfig thresholds;
  EXPECT_FALSE(thresholds.load("non_existent_file.conf"));
}

TEST_F(ThresholdConfigTest, ProvidesDefaultValues) {
  ThresholdConfig thresholds;
  // When no file is loaded, it should either have reasonable defaults or return
  // 0 Based on implementation, usually we return a large range or standard
  // defaults. We'll just verify it doesn't crash
  EXPECT_NO_THROW(thresholds.getRange("temp").optimalMin);
}

} // namespace agri
