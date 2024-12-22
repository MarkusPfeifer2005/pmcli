#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN  // necessary to run tests
#include "doctest.h"  // wget https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
#include "str_lib.h"

TEST_CASE ("test shape extraction") {
    std::string ipt;
    ipt = "Tile 1 x 2 with Bed Pattern	3069bpb0014";
    CHECK(getShape(ipt, true) == "1 x 2");
    CHECK(ipt == "Tile with Bed Pattern	3069bpb0014");
    ipt = "Panel 3 x 3 x 6 Corner Convex";
    CHECK(getShape(ipt, false) == "3 x 3 x 6");
    CHECK(ipt == "Panel 3 x 3 x 6 Corner Convex");
    ipt = "Barrel 2 x 2 x 2 26170";  // problematic behavior!
    CHECK(getShape(ipt, true) == "2 x 2 x 2 26170");
    CHECK(ipt == "Barrel");
    ipt = "Wheel Skateboard / Trolley";
    CHECK(getShape(ipt, true) == "");
    CHECK(ipt == "Wheel Skateboard / Trolley");
    ipt = "Windscreen 10 x 4 x 2 1/3 Canopy";
    CHECK(getShape(ipt, true) == "10 x 4 x 2 1/3");
    CHECK(ipt == "Windscreen Canopy");
    ipt = "Windscreen 10 x 4 x 21/3 Canopy";
    CHECK(getShape(ipt, false) == "10 x 4 x 21/3");
    CHECK(ipt == "Windscreen 10 x 4 x 21/3 Canopy");
    ipt = "Brick 1 / 2x2";
    CHECK(getShape(ipt, true) == "1 / 2 x 2");  // Spaces before and after '/' result in wrong shapes!
    CHECK(ipt == "Brick");
    ipt = "76766";
    CHECK(getShape(ipt, true) == "");
    CHECK(ipt == "76766");
}