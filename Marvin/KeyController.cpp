#include "KeyController.h"

#include <cstring>

namespace marvin {

KeyController::KeyController(unsigned long long timestamp) {
	ReleaseAll(); 
	for (KeyId i = 0; i < kKeyCount; i++) {
		timestamp_[i] = timestamp;
		duration_[i] = 0;
		rTrigger_[i] = false;
	}
}

void KeyController::Set(KeyId key, bool down) {
  if (key >= kKeyCount) return;

	  keys_[key] = down;
}

void KeyController::PressArrow(KeyId key, unsigned long long timestamp, unsigned long long pDuration, unsigned long long rDuration, bool release) {

}

void KeyController::Press(KeyId key, unsigned long long timestamp, unsigned long long duration) {
  if (key >= kKeyCount) return;
  //slows down arrow key inputs when switching between right and left
  if (key == 0x27 && rTrigger_[key] == true) {
	  if (timestamp - timestamp_[0x25] < duration_[0x25]) { return; }
  }
  if (key == 0x25 && rTrigger_[key] == true) {
	  if (timestamp - timestamp_[0x27] < duration_[0x27]) { return; }
  }

  timestamp_[key] = timestamp;
  duration_[key] = duration;
  rTrigger_[key] = false;
  keys_[key] = true;
}
void KeyController::Press(KeyId key) {
	if (key >= kKeyCount) return;

	duration_[key] = 0;
	rTrigger_[key] = false;
	keys_[key] = true;
}

bool KeyController::IsPressed(KeyId key) {
  if (key >= kKeyCount) return false;

  return keys_[key];
}

void KeyController::Release(KeyId key) {
  if (key >= kKeyCount) return;

  keys_[key] = false;
}

void KeyController::ReleaseAll() { memset(keys_, 0, sizeof(keys_)); }

void KeyController::Update(unsigned long long timestamp) {

	for (KeyId i = 0; i < kKeyCount; i++) {
		if (keys_[i]) {
			//for keys wityh duration timers, prevents them from being released until the timer expires (30-50ms)
			if (duration_[i] == 0 || (timestamp - timestamp_[i]) > duration_[i]) {
				keys_[i] = false;
				rTrigger_[i] = true;
			}
			if (i == 0x25 || i == 0x27) {
				keys_[i] = false;
				rTrigger_[i] = true;
			}
		}
	}
}

}  // namespace marvin
