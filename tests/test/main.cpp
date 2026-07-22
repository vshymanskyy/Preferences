#include <unity.h>

#if defined(NVS_USE_WIFININA)
  // For WiFiNINA compatibility tests
  #include <WiFiPreferences.h>
#else
  #include <Preferences.h>
#endif

void setUp(void) {
}

void tearDown(void) {
}


void test_bytes() {
  const int count = 5;
  static uint32_t sizes[count] = { 1, 2, 2, 50, 5 };
  static const char* bytes[count] = {
    "0", "12", "AB",
    "12345678901234567890123456789012345678901234567890",
    "Hello"
  };

  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  for (int i = 0; i < count; i++) {
    const char* data = bytes[i];
    uint32_t len  = sizes[i];
    uint8_t buf[128] = { 0, };

    TEST_ASSERT_EQUAL_UINT(len, prefs.putBytes("aaa", data, len));
    TEST_ASSERT_TRUE(prefs.isKey("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytesLength("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytes("aaa", buf, sizeof(buf)));
    if (len) {
      TEST_ASSERT_EQUAL_MEMORY(data, buf, len);
    }
  }

  // Do the same, but decreasing size
  for (int i = count-2; i >= 0; i--) {
    const char* data = bytes[i];
    uint32_t len  = sizes[i];
    uint8_t buf[128] = { 0, };

    TEST_ASSERT_EQUAL_UINT(len, prefs.putBytes("aaa", data, len));
    TEST_ASSERT_TRUE(prefs.isKey("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytesLength("aaa"));
    TEST_ASSERT_EQUAL_UINT(len, prefs.getBytes("aaa", buf, sizeof(buf)));
    if (len) {
      TEST_ASSERT_EQUAL_MEMORY(data, buf, len);
    }
  }

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_zero_bytes() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_EQUAL_UINT(0, prefs.putBytes("bytes", "", 0));
  TEST_ASSERT_TRUE(prefs.isKey("bytes"));
  TEST_ASSERT_EQUAL_UINT(0, prefs.getBytesLength("bytes"));

  TEST_ASSERT_EQUAL_UINT(0, prefs.putString("string", ""));
  TEST_ASSERT_TRUE(prefs.isKey("string"));
  TEST_ASSERT_EQUAL_STRING("", prefs.getString("string", "default").c_str());

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_string() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_EQUAL_UINT(7, prefs.putString("aaa", "value A"));
  TEST_ASSERT_EQUAL_UINT(7, prefs.putString("bbb", "value B"));

  char buffer[8];

  prefs.getString("aaa", buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("value A", buffer);

  prefs.getString("bbb", buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("value B", buffer);
}

void test_remove_key() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  // Add values and verify
  TEST_ASSERT_EQUAL_UINT(7, prefs.putString("aaa", "value A"));
  TEST_ASSERT_EQUAL_UINT(7, prefs.putString("bbb", "value B"));
  TEST_ASSERT_TRUE(prefs.isKey("aaa"));
  TEST_ASSERT_TRUE(prefs.isKey("bbb"));
  TEST_ASSERT_EQUAL_STRING("value A", prefs.getString("aaa").c_str());
  TEST_ASSERT_EQUAL_STRING("value B", prefs.getString("bbb").c_str());

  // Remove A, check that B is intact
  TEST_ASSERT_TRUE(prefs.remove("aaa"));
  TEST_ASSERT_FALSE(prefs.isKey("aaa"));
  TEST_ASSERT_TRUE(prefs.isKey("bbb"));
  TEST_ASSERT_EQUAL_STRING("", prefs.getString("aaa").c_str());
  TEST_ASSERT_EQUAL_STRING("value B", prefs.getString("bbb").c_str());

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_clear_namespace() {
  Preferences prefsA, prefsB;
  TEST_ASSERT_TRUE(prefsA.begin("testA"));
  TEST_ASSERT_TRUE(prefsB.begin("testB"));

  // Add values and verify
  TEST_ASSERT_EQUAL_UINT(7, prefsA.putString("value", "value A"));
  TEST_ASSERT_EQUAL_UINT(7, prefsB.putString("value", "value B"));
  TEST_ASSERT_EQUAL_STRING("value A", prefsA.getString("value").c_str());
  TEST_ASSERT_EQUAL_STRING("value B", prefsB.getString("value").c_str());

  // Clear A, check that B is intact
  TEST_ASSERT_TRUE(prefsA.clear());
  TEST_ASSERT_EQUAL_STRING("", prefsA.getString("value").c_str());
  TEST_ASSERT_EQUAL_STRING("value B", prefsB.getString("value").c_str());

  // Clear B
  TEST_ASSERT_TRUE(prefsB.clear());
  TEST_ASSERT_EQUAL_STRING("", prefsB.getString("value").c_str());

  // Add values and verify (again)
  TEST_ASSERT_EQUAL_UINT(9, prefsA.putString("value", "value AAA"));
  TEST_ASSERT_EQUAL_UINT(9, prefsB.putString("value", "value BBB"));
  TEST_ASSERT_EQUAL_STRING("value AAA", prefsA.getString("value").c_str());
  TEST_ASSERT_EQUAL_STRING("value BBB", prefsB.getString("value").c_str());

  TEST_ASSERT_TRUE(prefsA.clear());
  TEST_ASSERT_TRUE(prefsB.clear());
}

void test_utf8_key() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_EQUAL_UINT(4, prefs.putUInt("😁", 1234));
  TEST_ASSERT_EQUAL_UINT(1234, prefs.getUInt("😁"));

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_utf8_value() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_EQUAL_UINT(4, prefs.putString("unicode", "😁"));
  TEST_ASSERT_EQUAL_STRING("😁", prefs.getString("unicode").c_str());

  TEST_ASSERT_TRUE(prefs.clear());
}

// Regression test: getString() must never write past the caller's buffer.
// A 1-byte buffer only has room for the null terminator, which used to
// trigger an out-of-bounds write (see getBytes()'s maxLen==0 "query size"
// convention colliding with getString()'s maxLen-1 computation).
// Matches the real ESP32 API: on failure, the buffer is left completely
// untouched (not even null-terminated).
void test_get_string_tiny_buffer() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_EQUAL_UINT(5, prefs.putString("s", "hello"));

  char guarded[3] = { 'X', 'X', 'X' };
  size_t len = prefs.getString("s", guarded, 1);
  TEST_ASSERT_EQUAL_UINT(0, len);
  TEST_ASSERT_EQUAL_CHAR('X', guarded[0]); // untouched: proves no overflow
  TEST_ASSERT_EQUAL_CHAR('X', guarded[1]); // untouched: proves no overflow
  TEST_ASSERT_EQUAL_CHAR('X', guarded[2]); // untouched: proves no overflow

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_get_string_buffer_too_small() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_EQUAL_UINT(11, prefs.putString("s", "Hello World"));

  char guarded[8] = { 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X' };
  size_t len = prefs.getString("s", guarded, 6); // not enough room for the value
  TEST_ASSERT_EQUAL_UINT(0, len);
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_EQUAL_CHAR('X', guarded[i]); // untouched: proves no overflow
  }

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_get_bytes_too_small() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  uint8_t data[10];
  for (int i = 0; i < 10; i++) data[i] = i;
  TEST_ASSERT_EQUAL_UINT(10, prefs.putBytes("b", data, 10));

  uint8_t small[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
  TEST_ASSERT_EQUAL_UINT(0, prefs.getBytes("b", small, 4));
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_HEX8(0xAA, small[i]); // untouched: proves no overflow
  }

  TEST_ASSERT_TRUE(prefs.clear());
}

// Operations on a Preferences object that was never begin()'d must fail
// safely (return defaults / 0 / false) instead of touching storage.
void test_operations_before_begin() {
  Preferences prefs;

  TEST_ASSERT_FALSE(prefs.isKey("k"));
  TEST_ASSERT_EQUAL_UINT(0, prefs.getBytesLength("k"));
  TEST_ASSERT_EQUAL_INT(42, prefs.getInt("k", 42));
  TEST_ASSERT_EQUAL_STRING("def", prefs.getString("k", "def").c_str());

  uint8_t buf[4] = { 0 };
  TEST_ASSERT_EQUAL_UINT(0, prefs.getBytes("k", buf, sizeof(buf)));

  TEST_ASSERT_EQUAL_UINT(0, prefs.putInt("k", 1));
  TEST_ASSERT_FALSE(prefs.remove("k"));
  TEST_ASSERT_FALSE(prefs.clear());
}

// README: get*() operations don't fail if the existing value has a
// different type, as long as its size matches. A size mismatch (either
// direction) is treated like a missing key: the default is returned
// completely untouched, not partially filled. This matches the real ESP32
// API (NVS enforces size on read), so it runs unconditionally.
void test_size_mismatch_fails() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  uint8_t twoBytes[2] = { 0x11, 0x22 };
  TEST_ASSERT_EQUAL_UINT(2, prefs.putBytes("small", twoBytes, 2));
  int32_t value = prefs.getInt("small", (int32_t)0x7F7F7F7F); // wider read
  TEST_ASSERT_EQUAL_HEX32(0x7F7F7F7F, value); // doesn't fit: default preserved, not partially filled

  TEST_ASSERT_EQUAL_UINT(4, prefs.putInt("big", 0x11223344));
  int8_t small = prefs.getChar("big", (int8_t)0x5A); // narrower read
  TEST_ASSERT_EQUAL_INT8(0x5A, small); // doesn't fit: default preserved

  TEST_ASSERT_TRUE(prefs.clear());
}

// README: same-size types may reinterpret each other's raw bytes (e.g.
// getFloat() can read a value written with putUInt()). Real ESP32's NVS
// tags entries with a specific type and enforces it strictly, so this is a
// deliberate divergence and only runs against this library's backends.
void test_type_reinterpret_same_size() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_EQUAL_UINT(4, prefs.putUInt("v", 0x3F800000)); // 1.0f as raw bits
  TEST_ASSERT_EQUAL_FLOAT(1.0f, prefs.getFloat("v", -1.0f));

  TEST_ASSERT_EQUAL_UINT(4, prefs.putInt("w", -1));
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, prefs.getUInt("w", 0));

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_readonly() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_EQUAL_UINT(7, prefs.putString("val", "initial"));
  prefs.end();

  Preferences ro;
  TEST_ASSERT_TRUE(ro.begin("test", true));

  // Reads still work in read-only mode
  TEST_ASSERT_EQUAL_STRING("initial", ro.getString("val").c_str());

  // Writes must fail and leave existing data untouched
  TEST_ASSERT_EQUAL_UINT(0, ro.putString("val", "changed"));
  TEST_ASSERT_EQUAL_UINT(0, ro.putInt("newkey", 42));
  TEST_ASSERT_FALSE(ro.remove("val"));
  TEST_ASSERT_FALSE(ro.clear());

  TEST_ASSERT_EQUAL_STRING("initial", ro.getString("val").c_str());
  TEST_ASSERT_FALSE(ro.isKey("newkey"));
  ro.end();

  Preferences cleanup;
  TEST_ASSERT_TRUE(cleanup.begin("test"));
  TEST_ASSERT_TRUE(cleanup.clear());
}

void test_begin_twice() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_FALSE(prefs.begin("test")); // already started

  TEST_ASSERT_TRUE(prefs.clear());
  prefs.end();

  // After end(), begin() should work again
  TEST_ASSERT_TRUE(prefs.begin("test"));
  TEST_ASSERT_TRUE(prefs.clear());
}

void test_put_string_object_overload() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  String s = "object overload";
  TEST_ASSERT_EQUAL_UINT(s.length(), prefs.putString("obj", s));
  TEST_ASSERT_EQUAL_STRING("object overload", prefs.getString("obj").c_str());

  TEST_ASSERT_TRUE(prefs.clear());
}

// Writing the exact same value again exercises the "data unchanged, skip
// the flash write" fast path and must still report success.
void test_put_same_value_twice() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  uint8_t data[4] = { 1, 2, 3, 4 };
  TEST_ASSERT_EQUAL_UINT(4, prefs.putBytes("k", data, 4));
  TEST_ASSERT_EQUAL_UINT(4, prefs.putBytes("k", data, 4));

  uint8_t out[4] = { 0 };
  TEST_ASSERT_EQUAL_UINT(4, prefs.getBytes("k", out, 4));
  TEST_ASSERT_EQUAL_MEMORY(data, out, 4);

  TEST_ASSERT_TRUE(prefs.clear());
}

void test_missing_key_defaults() {
  Preferences prefs;
  TEST_ASSERT_TRUE(prefs.begin("test"));

  TEST_ASSERT_FALSE(prefs.isKey("missing"));
  TEST_ASSERT_EQUAL_INT(-7, prefs.getInt("missing", -7));
  TEST_ASSERT_EQUAL_STRING("fallback", prefs.getString("missing", "fallback").c_str());
  TEST_ASSERT_FALSE(prefs.getBool("missing", false));
  TEST_ASSERT_TRUE(prefs.getBool("missing2", true));
  TEST_ASSERT_EQUAL_UINT(0, prefs.getBytesLength("missing"));

  // char*-buffer overload: a missing key must leave the buffer untouched,
  // same as a buffer that's too small.
  char guarded[4] = { 'X', 'X', 'X', 'X' };
  TEST_ASSERT_EQUAL_UINT(0, prefs.getString("missing", guarded, sizeof(guarded)));
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_CHAR('X', guarded[i]);
  }

  TEST_ASSERT_TRUE(prefs.clear());
}

int runUnityTests(void) {
  UNITY_BEGIN();

  RUN_TEST(test_bytes);
  RUN_TEST(test_string);
  RUN_TEST(test_utf8_key);
  RUN_TEST(test_utf8_value);
  RUN_TEST(test_get_string_tiny_buffer);
  RUN_TEST(test_get_string_buffer_too_small);
  RUN_TEST(test_get_bytes_too_small);
  RUN_TEST(test_operations_before_begin);
  RUN_TEST(test_size_mismatch_fails);
  RUN_TEST(test_readonly);
  RUN_TEST(test_begin_twice);
  RUN_TEST(test_put_string_object_overload);
  RUN_TEST(test_put_same_value_twice);
  RUN_TEST(test_missing_key_defaults);
  RUN_TEST(test_remove_key);
  RUN_TEST(test_clear_namespace);
#if !(defined(ESP32) || defined(NVS_USE_WIFININA))
  RUN_TEST(test_zero_bytes);
  RUN_TEST(test_type_reinterpret_same_size);
#endif

  return UNITY_END();
}


#if defined(ARDUINO)
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println();

  runUnityTests();
}

void loop() {
  delay(100);
}
#else
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  return runUnityTests();
}
#endif
