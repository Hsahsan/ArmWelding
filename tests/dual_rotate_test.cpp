#include <cassert>
#include <cstdint>
#include <iostream>

struct Axis {
    std::int32_t origin{};
    std::int32_t target{};
    std::int32_t displacement{};
    bool fault{};
    bool quick_stop{};
};

static void plan_opposite(Axis& a, Axis& b, std::int32_t displacement) {
    a.displacement = displacement;
    b.displacement = displacement;
    a.target = a.origin + displacement;
    b.target = b.origin - displacement;
}

static void stop_both(Axis& a, Axis& b) {
    a.quick_stop = true;
    b.quick_stop = true;
}

static void test_targets_are_simultaneous_and_opposite() {
    Axis a{1000, 0, 0, false, false};
    Axis b{-2000, 0, 0, false, false};
    plan_opposite(a, b, 131072);
    assert(a.target == 132072);
    assert(b.target == -133072);
    assert(a.displacement == b.displacement);
}

static void test_negative_displacement_preserves_opposition() {
    Axis a{1000, 0, 0, false, false};
    Axis b{-2000, 0, 0, false, false};
    plan_opposite(a, b, -500);
    assert(a.target == 500);
    assert(b.target == -1500);
}

static void test_fault_stops_both_axes() {
    Axis a{0, 100, 100, true, false};
    Axis b{0, -100, 100, false, false};
    stop_both(a, b);
    assert(a.quick_stop);
    assert(b.quick_stop);
}

int main() {
    test_targets_are_simultaneous_and_opposite();
    test_negative_displacement_preserves_opposition();
    test_fault_stops_both_axes();
    std::cout << "dual_rotate_test: PASS\n";
    return 0;
}
