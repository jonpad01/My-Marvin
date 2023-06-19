#pragma once


namespace marvin {

class HSBlackboard {
 private:
  AnchorPlayer anchor;
  const Player* enemy_anchor;

  int team_lanc_count;
  int team_spid_count;
  int flagger_count;

  bool update_lancs_flag;



  public: 

  HSBlackboard() { 
    enemy_anchor = nullptr;

    team_lanc_count = 0;
    team_spid_count = 0;
    flagger_count = 0;
    update_lancs_flag = true;


  }

  const AnchorPlayer& GetAnchor() { return anchor; }
  void SetAnchor(const AnchorPlayer& player) { anchor = player; } 

  const Player* GetEnemyAnchor() { return enemy_anchor; }
  void SetEnemyAnchor(const Player* player) { enemy_anchor = player; } 

  int GetTeamLancCount() { return team_lanc_count; }
  void SetTeamLancCount(int count) { team_lanc_count = count; } 

  int GetTeamSpidCount() { return team_spid_count; }
  void SetTeamSpidCount(int count) { team_spid_count = count; } 

  int GetFlaggerCount() { return flagger_count; }
  void SetFlaggerCount(int count) { flagger_count = count; } 

  bool GetUpdateLancsFlag() { return update_lancs_flag; }
  void SetUpdateLancsFlag(bool count) { update_lancs_flag = count; } 

};

}  // namespace marvin