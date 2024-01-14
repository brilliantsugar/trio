#include <trio.h>

#include <relacy/relacy.hpp>

// Will create 2 "threads":
// One for consumer, one for producer
static constexpr size_t kThreads = 2;

// Each buffer is just a single integer.
// Testing for:
// - Order of updates is the same on producer and consumer.
// - Last written value by producer is never lost
// - Other conditions that relacy checks automatically:
//   data races, deadlocks, livelocks.
struct TrioTest : rl::test_suite<TrioTest, kThreads> {
    static constexpr size_t kLastNumber = 25;
    bs::Trio<rl::var<int>> trio{rl::var<int>{0}, rl::var<int>{0}, rl::var<int>{0}};

    void thread(unsigned thread_index) {
        if (thread_index == 0) { // Producer
            // Writing 25 integers
            for (int i = 1; i <= kLastNumber; ++i) {
                auto rlval = trio.Write()($);
                rlval.store(i);
                trio.Commit();
            }
        } else { // Consumer
            // Read until we get 25, if 25 is never read
            // relacy will report livelock.
            int prev_read = 0;
            while (prev_read != kLastNumber) {
                auto [rlval, changed] = trio.Read();
                int val = rlval($);
                if (changed) {
                    // Check order: new value must be > than the previous
                    // It will be > when it's new value.
                    RL_ASSERT(val > prev_read);
                    prev_read = val;
                } else {
                    RL_ASSERT(val == prev_read);
                }
            }
        }
    }
};

int main() {
    rl::simulate<TrioTest>();
    return 0;
}
