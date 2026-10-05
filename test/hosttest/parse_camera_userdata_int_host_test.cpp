#include "parse_camera_userdata_int.h"

#include <iostream>
#include <string>

using OHOS::CameraStandard::ParseCameraUserDataInt;

static int g_fail = 0;

static void ExpectTrue(const char *name, bool ok)
{
    if (!ok) {
        std::cerr << "FAIL " << name << "\n";
        ++g_fail;
    }
}

static void ExpectFalse(const char *name, bool ok)
{
    ExpectTrue(name, !ok);
}

static void ExpectEq(const char *name, int32_t got, int32_t want)
{
    if (got != want) {
        std::cerr << "FAIL " << name << " got=" << got << " want=" << want << "\n";
        ++g_fail;
    }
}

int main()
{
    int32_t out = -999;
    ExpectTrue("0", ParseCameraUserDataInt("0", out));
    ExpectEq("0val", out, 0);
    ExpectTrue("123", ParseCameraUserDataInt("123", out));
    ExpectEq("123val", out, 123);
    ExpectTrue("-1", ParseCameraUserDataInt("-1", out));
    ExpectEq("-1val", out, -1);
    ExpectTrue("INT_MAX", ParseCameraUserDataInt("2147483647", out));
    ExpectEq("INT_MAXval", out, 2147483647);

    ExpectFalse("empty", ParseCameraUserDataInt("", out));
    ExpectFalse("abc", ParseCameraUserDataInt("abc", out));
    ExpectFalse("12a", ParseCameraUserDataInt("12a", out));
    ExpectFalse("space", ParseCameraUserDataInt(" 12", out));
    ExpectFalse("overflow", ParseCameraUserDataInt("2147483648", out));
    ExpectFalse("huge", ParseCameraUserDataInt("9999999999999999999", out));
    ExpectFalse("neg_overflow", ParseCameraUserDataInt("-2147483649", out));

    bool threwEmpty = false;
    try {
        (void)std::stoi(std::string(""));
    } catch (...) {
        threwEmpty = true;
    }
    ExpectTrue("stoi_empty_throws", threwEmpty);

    bool threwOvf = false;
    try {
        (void)std::stoi(std::string("2147483648"));
    } catch (...) {
        threwOvf = true;
    }
    ExpectTrue("stoi_overflow_throws", threwOvf);

    if (g_fail != 0) {
        std::cerr << g_fail << " failures\n";
        return 1;
    }
    std::cout << "camera_standard ParseCameraUserDataInt host test passed\n";
    return 0;
}
