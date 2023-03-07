#include <unity.h>

#include <Preferences.h>

void test_bytes() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  static const char* bytes[] = {
    "", "1", "12", "AB", "Hello",
    "12345678901234567890123456789012345678901234567890"
  };
  static uint32_t sizes[] = { 0, 1, 2, 2, 5, 50 };

  uint8_t buf[128];
  for (int i = 0; i < 6; i++) {
    const char* data = bytes[i];
    uint32_t len  = sizes[i];
    TEST_ASSERT_EQUAL_UINT(len, prefs.putBytes("aaa", data, len));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytesLength("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytes("aaa", buf, sizeof(buf)));
    if (len) {
      TEST_ASSERT_EQUAL_MEMORY(data, buf, len);
    }
  }

  // Do the same, but decreasing size
  for (int i = 4; i >= 0; i--) {
    const char* data = bytes[i];
    uint32_t len  = sizes[i];
    TEST_ASSERT_EQUAL_UINT(len, prefs.putBytes("aaa", data, len));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytesLength("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytes("aaa", buf, sizeof(buf)));
    if (len) {
      TEST_ASSERT_EQUAL_MEMORY(data, buf, len);
    }
  }

}

void test_utf8_key() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_EQUAL_UINT(4, prefs.putUInt("😁", 1234));
  TEST_ASSERT_EQUAL_UINT(1234, prefs.getUInt("😁"));
}

void test_utf8_value() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  prefs.putString("unicode", "😁");
  String got = prefs.getString("unicode");
  TEST_ASSERT_EQUAL_STRING("😁", got.c_str());
}

int runUnityTests(void) {
  UNITY_BEGIN();

  RUN_TEST(test_bytes);
  RUN_TEST(test_utf8_key);
  RUN_TEST(test_utf8_value);

  return UNITY_END();
}


void setup() {
  delay(1000);

  runUnityTests();
}

void loop() {
  delay(100);
}
