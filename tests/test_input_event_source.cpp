#include "coreflow/input_event_source.hpp"

#include <cassert>
#include <linux/input.h>

using namespace coreflow;

int main() {
    assert(is_touchscreen_capability(true, false, true, true, true, true));
    assert(is_touchscreen_capability(false, false, true, true, true, false));
    assert(!is_touchscreen_capability(false, true, true, true, true, false));
    assert(!is_touchscreen_capability(false, false, true, false, true, false));
    assert(!is_touchscreen_capability(true, false, true, true, false, false));

    TouchInputState state{};
    FrameEvent output{};
    const auto event = [](uint16_t type, uint16_t code, int32_t value) {
        input_event result{};
        result.type = type;
        result.code = code;
        result.value = value;
        return result;
    };

    assert(!consume_touch_input_event(state, event(EV_KEY, BTN_TOUCH, 1), 1'000'000ULL, output));
    assert(consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 2'000'000ULL, output));
    assert(output.kind == FrameEventKind::TouchBurst && output.burst_duration_ms == 80U);

    assert(!consume_touch_input_event(state, event(EV_ABS, ABS_X, 100), 10'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 20'000'000ULL, output));
    assert(consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 60'000'000ULL, output));

    assert(!consume_touch_input_event(state, event(EV_KEY, BTN_TOUCH, 0), 61'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_ABS, ABS_X, 101), 120'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 121'000'000ULL, output));

    TouchInputState mt{};
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 0), 1, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, 17), 2, output));
    assert(consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 3, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 1), 4, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, 18), 5, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 0), 6, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, -1), 7, output));
    assert(!consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 8, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 1), 9, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, -1), 10, output));
    assert(!consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 11, output));

    TouchInputState dropped{};
    assert(!consume_touch_input_event(dropped, event(EV_KEY, BTN_TOUCH, 1), 1, output));
    assert(!consume_touch_input_event(dropped, event(EV_SYN, SYN_DROPPED, 0), 2, output));
    assert(!consume_touch_input_event(dropped, event(EV_SYN, SYN_REPORT, 0), 3, output));
    assert(!consume_touch_input_event(dropped, event(EV_KEY, BTN_TOUCH, 1), 4, output));
    assert(consume_touch_input_event(dropped, event(EV_SYN, SYN_REPORT, 0), 5, output));
    return 0;
}
