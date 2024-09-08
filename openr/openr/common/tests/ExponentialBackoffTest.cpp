/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <signal.h>
#include <unistd.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <openr/common/ExponentialBackoff.h>

TEST(ExponentialBackoffTest, AbortAtMaxTest) {
  openr::ExponentialBackoff<std::chrono::milliseconds> timer(
      std::chrono::milliseconds(1), std::chrono::milliseconds(2), true);
  timer.reportError();
  timer.reportError();
  EXPECT_EXIT(
      timer.reportError(),
      ::testing::KilledBySignal(SIGABRT),
      "Max back-off reached");
}

TEST(ExponentialBackoffTest, ApiTest) {
  openr::ExponentialBackoff<std::chrono::milliseconds> timer(
      std::chrono::milliseconds(2), std::chrono::milliseconds(10));
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  timer.reportSuccess();
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  // Report error and timer should become 2
  timer.reportError();
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(1));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(2));

  // Report error again and timer should become 4
  timer.reportError();
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(3));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(4));

  // Report error again and timer should become 8
  timer.reportError();
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(7));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(8));

  // Report error again and timer should become 10
  timer.reportError();
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_TRUE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(9));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(10));

  // Report success and things should fall back to normal
  timer.reportSuccess();
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  // Increase wait timer
  timer.reportError(); // 2ms
  timer.reportError(); // 4ms
  timer.reportError(); // 8ms
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(7));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(8));

  // Sleep for 8ms and then wake up to check timer is clear
  /* sleep override */
  usleep(8000);
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));
}

TEST(ExponentialBackoffTest, SameInitialAndMaxBackoffTest) {
  openr::ExponentialBackoff<std::chrono::milliseconds> timer(
      std::chrono::milliseconds(2), std::chrono::milliseconds(2));
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  timer.reportSuccess();
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  // Report error and timer should become 2
  timer.reportError();
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_TRUE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(1));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(2));

  // Report success and things should fall back to normal
  timer.reportSuccess();
  EXPECT_TRUE(timer.canTryNow());
  EXPECT_FALSE(timer.atMaxBackoff());
  EXPECT_EQ(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(0));

  // Increase wait timer
  timer.reportError(); // 2ms
  timer.reportError(); // 2ms
  timer.reportError(); // 2ms
  EXPECT_FALSE(timer.canTryNow());
  EXPECT_TRUE(timer.atMaxBackoff());
  EXPECT_GE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(1));
  EXPECT_LE(timer.getTimeRemainingUntilRetry(), std::chrono::milliseconds(2));
}

int
main(int argc, char** argv) {
  // Basic initialization
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;

  // Run the tests
  return RUN_ALL_TESTS();
}
