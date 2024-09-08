/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <chrono>

namespace openr {

/**
 * Exponential backoff for generic keys using trial and error.
 *
 * This utility implements exponential backoff for keeping track of when to
 * retry something. A separate error count is maintained for each "key" used,
 * so that a different backoff can generated for each item.
 */
template <typename Duration>
class ExponentialBackoff {
 public:
  /**
   * Make ExponentialBackoff default constructible. Though it is not very
   * much usable at all unless reassigned with valid one.
   */
  ExponentialBackoff();

  /**
   * @param initialBackoff  The length of time to wait before retrying
   *                        after the first error.
   * @param maxBackoff      The maximum backoff period to use.
   * @param isAbortAtMax    Whether to fail and abort() at maximum.
   *
   *                        This should be set for critical services, to
   *                        ensure a problem is escalated to higher-level
   *                        service managers for further action.
   *
   *                        Defaults to false.
   */
  ExponentialBackoff(
      Duration initialBackoff, Duration maxBackoff, bool isAbortAtMax = false);

  /**
   * Should we wait or not?
   *
   * If there was an error, we want to wait for the backoff period specified
   * in the constructor. If there is still an error then keep doubling the
   * backoff time, up to the specified maximum backoff.
   */
  bool canTryNow() const;

  /**
   * Clear backoff period
   */
  void reportSuccess();

  /**
   * Note that we should back off more
   */
  void reportError();

  void
  reportStatus(bool status) {
    if (status) {
      reportSuccess();
    } else {
      reportError();
    }
  }

  /**
   * Have we reached the maximum backoff?
   */
  bool atMaxBackoff() const;

  /**
   * Get the time remaining until next retry
   */
  Duration getTimeRemainingUntilRetry() const;

  Duration
  getCurrentBackoff() const {
    return currentBackoff_;
  }

  /**
   * Get latest error timestamp
   */
  std::chrono::steady_clock::time_point getLastErrorTime() const;

  /**
   * Getters for backoff time
   */
  Duration getInitialBackoff() const;
  Duration getMaxBackoff() const;
  /**
   * Getter for AbortAtMax
   */
  bool getIsAbortAtMax() const;

 private:
  Duration initialBackoff_;
  Duration maxBackoff_;

  // Current backoff. If things are good then it is Duration(0)
  Duration currentBackoff_;

  bool isAbortAtMax_ = false;

  // Time point of last error
  std::chrono::steady_clock::time_point lastErrorTime_;
};
} // namespace openr
