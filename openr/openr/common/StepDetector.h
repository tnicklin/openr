/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/stats/BucketedTimeSeries.h>
#include <openr/if/gen-cpp2/OpenrConfig_types.h>

namespace openr {

/*
 * This class detects abrupt changes, i.e., steps, in the mean level of a time
 * series or signal. Often, the step is small and the time series is corrupted
 * by some kind of noise. We average the time series over two sliding windows:
 * one fast to capture short-term mean and one slow for long-term. When the
 * fast average significantly differs from that of the slow window, we signal
 * a step. We compute the difference as absolute ratio of change over slow
 * mean, which is expected to be a plateau around 0 dotted with mountains caused
 * by steps on the time horizon. When it surpasses an upper threshold, we know
 * we are on the rising edge of a step. We wait till it drops below a lower
 * threshold on the falling edge to signal the step and use the fast mean at
 * that moment as the new mean.
 * Special case: when time series changes in a staircase way. If each stair is
 * small, the previous detetion algorithm may fail. We add an absolute threshold
 * to catch this case.
 * Notes: we assume the underlying time series is stable for longer than slow
 * sliding window between steps.
 */
template <typename ValueType, typename TimeType>
class StepDetector {
 public:
  StepDetector(
      // step detector config from openr config
      const thrift::StepDetectorConfig& stepConfig,
      // interval time series is sampled
      TimeType samplePeriod,
      // callback when step is detected
      std::function<void(const ValueType&)> stepCb)
      : fastWndSize_(*stepConfig.fast_window_size()),
        slowWndSize_(*stepConfig.slow_window_size()),
        loThreshold_(*stepConfig.lower_threshold()),
        hiThreshold_(*stepConfig.upper_threshold()),
        absThreshold_(*stepConfig.ads_threshold()),
        fastSlideWindow_(fastWndSize_, samplePeriod * fastWndSize_),
        slowSlideWindow_(slowWndSize_, samplePeriod * slowWndSize_),
        stepCb_(std::move(stepCb)) {
    CHECK_LT(loThreshold_, hiThreshold_);
    CHECK_LT(fastWndSize_, slowWndSize_);
  }

  // add the value 'val' at time 'now' to both fast and slow sliding window
  bool
  addValue(TimeType now, const ValueType& val) {
    bool fastSuccess = fastSlideWindow_.addValue(now, val);
    bool slowSuccess = slowSlideWindow_.addValue(now, val);

    auto fastAvg = fastSlideWindow_.avg();
    auto slowAvg = slowSlideWindow_.avg();

    // init last average if not initialized and we gather enough samples
    if (!lastAvgInit_ && slowSlideWindow_.count() >= slowWndSize_ / 2) {
      lastAvg_ = slowAvg;
      lastAvgInit_ = true;
    }

    if (!slowAvg) {
      throw std::runtime_error("Divide by zero");
    }

    // forcefully use double for improved accuracy
    auto diff =
        std::abs((fastAvg - slowAvg) / static_cast<double>(slowAvg)) * 100;

    // state machine transition
    if (inTransit_) {
      if (diff <= loThreshold_) {
        // falling edge
        inTransit_ = false;
        VLOG(4) << "Step detected at time: " << now.count()
                << ", new mean: " << fastAvg;
        // report fast average since slow average may not have caught up with
        // new mean yet
        stepCb_(fastAvg);
        lastAvg_ = fastAvg;
        lastAvgInit_ = true;
        return fastSuccess && slowSuccess;
      }
    } else {
      if (diff >= hiThreshold_) {
        // rising edge
        inTransit_ = true;
      }
    }

    // detect slow boiling, i.e., gradual change, missed by state machine
    // only check when time series is stable, e.g., slow and fast mean are close
    if (diff <= loThreshold_ && lastAvgInit_ &&
        std::abs(slowAvg - lastAvg_) >= absThreshold_) {
      VLOG(4) << "Step detected at time: " << now.count()
              << ", new mean: " << slowAvg;
      // report slow average because it's more accurate
      stepCb_(slowAvg);
      lastAvg_ = slowAvg;
    }

    return fastSuccess && slowSuccess;
  }

 private:
  // StepDetector is non-copyable
  StepDetector(StepDetector const&) = delete;
  StepDetector& operator=(StepDetector const&) = delete;

  // fast sliding window size
  const uint64_t fastWndSize_{0};

  // slow sliding window size
  const uint64_t slowWndSize_{0};

  // lower threshold, in percentage
  const uint32_t loThreshold_{0};

  // upper threshold, in percentage
  const uint32_t hiThreshold_{0};

  // absolute step threshold to detect gradual change
  const ValueType absThreshold_{0};

  // fast sliding window
  folly::BucketedTimeSeries<ValueType, folly::LegacyStatsClock<TimeType>>
      fastSlideWindow_;

  // slow sliding window size
  folly::BucketedTimeSeries<ValueType, folly::LegacyStatsClock<TimeType>>
      slowSlideWindow_;

  // callback when step is detected
  const std::function<void(const ValueType&)> stepCb_{nullptr};

  // last reported average when detecting a step
  ValueType lastAvg_{0};

  // is lastAvg_ initialized
  bool lastAvgInit_{false};

  // current state of time series, between upper threshold on the rising edge
  // and lower threshold on the falling
  bool inTransit_{false};
};
} // namespace openr
