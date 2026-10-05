#include "FirebaseClient.hpp"
#include <gtest/gtest.h>


namespace agri {

class FirebaseClientTest : public ::testing::Test {
protected:
  FirebaseClient fb{"https://invalid-nonexistent-firebase-url.firebaseio.com",
                    "fake_key"};
};

TEST_F(FirebaseClientTest, InitDoesNotCrash) { EXPECT_NO_THROW(fb.init()); }

TEST_F(FirebaseClientTest, HandlesNetworkErrorsGracefully) {
  fb.init();

  SensorData data;
  data.temperature = 25.0;

  // This should fail to push, but shouldn't crash the program
  bool result = fb.pushSensorData("farm1", data);
  EXPECT_FALSE(result);
}

TEST_F(FirebaseClientTest, GetLatestReturnsNulloptOnFailure) {
  fb.init();
  auto result = fb.getLatest("farm1");
  EXPECT_FALSE(result.has_value());
}

} // namespace agri
