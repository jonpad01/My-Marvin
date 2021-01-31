#pragma once

namespace marvin {

using KeyId = int;
constexpr KeyId kKeyCount = 256;

class KeyController {
 public:
  KeyController(unsigned long long timestamp);

  void Set(KeyId key, bool down);
  void PressArrow(KeyId key, unsigned long long timestamp, unsigned long long pDuration, unsigned long long rDuration, bool release);
  void Press(KeyId key, unsigned long long timestamp, unsigned long long duration);
  void Press(KeyId key);
  void Release(KeyId key);
  void ReleaseAll();
  void Update(unsigned long long timestamp);

  bool IsPressed(KeyId key);

 private:
  bool keys_[kKeyCount];
  bool rTrigger_[kKeyCount];
  unsigned long long timestamp_[kKeyCount];
  unsigned long long duration_[kKeyCount];
};

}  // namespace marvin
