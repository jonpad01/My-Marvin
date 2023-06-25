#pragma once


namespace marvin {

class HSBlackboard {
 private:
  AnchorPlayer anchor;
  const Player* enemy_anchor;

  int flagger_count;

  bool update_lancs_flag;
  bool can_flag;



  public: 

  HSBlackboard() { 
    enemy_anchor = nullptr;

    flagger_count = 0;

    update_lancs_flag = true;
    can_flag = false;

  }

  const AnchorPlayer& GetAnchor() { return anchor; }
  void SetAnchor(const AnchorPlayer& player) { anchor = player; } 

  const Player* GetEnemyAnchor() { return enemy_anchor; }
  void SetEnemyAnchor(const Player* player) { enemy_anchor = player; } 

  int GetFlaggerCount() { return flagger_count; }
  void SetFlaggerCount(int count) { flagger_count = count; } 

  bool GetUpdateLancsFlag() { return update_lancs_flag; }
  void SetUpdateLancsFlag(bool count) { update_lancs_flag = count; } 

  bool GetCanFlag() { return can_flag; }
  void SetCanFlag(bool value) { can_flag = value; } 

};

}  // namespace marvin