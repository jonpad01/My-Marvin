#pragma once

#include <cstdint>

#include "MapCoord.h"

namespace marvin {

#pragma pack(push, 1)
// Per-ship settings
class ShipSettings {
 public:

  float GetRadius() const {
    int r = Radius == 0 ? 14 : Radius;

    return r / 16.0f;
  }

  float GetMultiFireAngle() const { return MultiFireAngle / (40000.0f / 360.0f); }

  float GetBulletSpeed() const { return BulletSpeed / 10.0f / 16.0f; }

  float GetBulletFireDelay() const { return BulletFireDelay / 100.0f; }

  float GetMultiFireDelay() const { return MultiFireDelay / 100.0f; }

  // gets the gun level
  float GetMaxGuns() const { return (float)(MaxGuns & 3); }

  float GetBombSpeed() const { return BombSpeed / 10.0f / 16.0f; }

  float GetBombFireDelay() const { return BombFireDelay / 100.0f; }

  float GetLandmineFireDelay() const { return LandmineFireDelay / 100.0f; }

  // gets the bomb level
  float GetMaxBombs() const { return (float)(MaxBombs & 3); }

  bool HasDoubleBarrel() const { return (DoubleBarrel & 1) != 0; }

  float GetInitialSpeed() const { return InitialSpeed / 10.0f / 16.0f; }
  float GetMaximumSpeed() const { return MaximumSpeed / 10.0f / 16.0f; }

  float GetInitialThrust() const { return InitialThrust * 10.0f / 16.0f; }
  float GetMaximumThrust() const { return MaximumThrust * 10.0f / 16.0f; }

  // changed from  / 200.f as / 400.f converts to rotations/second
  float GetInitialRotation() const { return (float)InitialRotation / 400.0f; }
  float GetMaximumRotation() const { return (float)MaximumRotation / 400.0f; }

  // How long Super lasts on the ship (in ticks)
  uint32_t SuperTime;
  // How long Super lasts on the ship (in ticks)
  uint32_t ShieldsTime;
  // How strong of an effect the wormhole has on this ship (0 = none)
  int16_t Gravity;
  // Ship are allowed to move faster than their maximum speed while
  // effected by a wormhole.This determines how much faster they can
  // go (0 = no extra speed)
  int16_t GravityTopSpeed;
  // Amount of energy it takes a ship to fire a single L1 bullet
  uint16_t BulletFireEnergy;
  // Amount of energy it takes a ship to fire multifire L1 bullets
  uint16_t MultiFireEnergy;
  // Amount of energy it takes a ship to fire a single bomb
  uint16_t BombFireEnergy;
  // Extra amount of energy it takes a ship to fire an upgraded bomb.
  // i.e. L2 = BombFireEnergy + BombFireEnergyUpgrade
  uint16_t BombFireEnergyUpgrade;
  // Amount of energy it takes a ship to place a single L1 mine
  uint16_t LandmineFireEnergy;
  // Extra amount of energy it takes to place an upgraded landmine.
  // i.e. L2 = LandmineFireEnergy + LandmineFireEnergyUpgrade
  uint16_t LandmineFireEnergyUpgrade;

 private:
  // How fast bullets travel
  uint16_t BulletSpeed;
  // How fast bombs travel
  uint16_t BombSpeed;

  struct {
    // If ship can see bombs on radar(0 = Disabled, 1 = All, 2 = L2 and up,
    // 3 = L3 and up, 4 = L4 bombs only)
    uint16_t SeeBombLevel : 2;
    // If firing bullets, bombs, or thors is disabled after using afterburners
    // (1 = enabled)
    uint16_t DisableFastShooting : 1;
    // The ship's radius from center to outside, in pixels.
    uint16_t Radius : 8;
    uint16_t padding : 5;
  };
  // Angle spread between multi-fire bullets and standard forward
  // firing bullets(0 = 0 degrees, 111.111111 = 1 degree, 222.22222 = 2 degrees, etc, 1000 = 1 ship - rotation - point)
  // conversion to degrees would be 40000 / 360
  // range is a number between 0 and 39999
  uint16_t MultiFireAngle;

 public:
  // Amount of energy required to have 'Cloak' activated (thousanths per tick)
  uint16_t CloakEnergy;
  // Amount of energy required to have 'Stealth' activated (thousanths per tick)
  uint16_t StealthEnergy;
  // Amount of energy required to have 'Anti-Warp' activated (thousanths per
  // tick)
  uint16_t AntiWarpEnergy;
  // Amount of energy required to have 'X-Radar' activated (thousanths per tick)
  uint16_t XRadarEnergy;

 private:
  // Maximum rotation rate of the ship (0 = can't rotate, 400 = full rotation in
  // 1 second)
  uint16_t MaximumRotation;
  // Maximum thrust of ship (0 = none)
  uint16_t MaximumThrust;
  // Maximum speed of ship (0 = can't move)
  uint16_t MaximumSpeed;

 public:
  // Maximum recharge rate, or how quickly this ship recharges its energy
  uint16_t MaximumRecharge;
  // Maximum amount of energy that the ship can have
  uint16_t MaximumEnergy;
  // Initial rotation rate of the ship (0 = can't rotate, 400 = full rotation in
  // 1 second)

 private:
  uint16_t InitialRotation;
  // Initial thrust of ship (0 = none)
  uint16_t InitialThrust;
  // Initial speed of ship (0 = can't move)
  uint16_t InitialSpeed;

 public:
  // Initial recharge rate, or how quickly this ship recharges its energy
  uint16_t InitialRecharge;
  // Initial amount of energy that the ship can have
  uint16_t InitialEnergy;
  // Amount added per 'Rotation' Prize
  uint16_t UpgradeRotation;
  // Amount added per 'Thruster' Prize
  uint16_t UpgradeThrust;
  // Amount added per 'Speed' Prize
  uint16_t UpgradeSpeed;
  // Amount added per 'Recharge Rate' Prize
  uint16_t UpgradeRecharge;
  // Amount added per 'Energy Upgrade' Prize
  uint16_t UpgradeEnergy;
  // Amount of energy required to have 'Afterburners' activated
  uint16_t AfterburnerEnergy;
  // Amount of back-thrust you receive when firing a bomb
  uint16_t BombThrust;
  // How fast the burst shrapnel is for this ship
  uint16_t BurstSpeed;
  // Amount the ship's thrust is decreased with a turret riding
  int16_t TurretThrustPenalty;
  // Amount the ship's speed is decreased with a turret riding
  int16_t TurretSpeedPenalty;

 private:
  // Delay that ship waits after a bullet is fired until another
  // weapon may be fired(in ticks)
  uint16_t BulletFireDelay;
  // Delay that ship waits after a multifire bullet is fired until
  // another weapon may be fired(in ticks)
  uint16_t MultiFireDelay;
  // delay that ship waits after a bomb is fired until another weapon
  // may be fired(in ticks)
  uint16_t BombFireDelay;
  // Delay that ship waits after a mine is fired until another weapon
  // may be fired(in ticks)
  uint16_t LandmineFireDelay;

 public:
  // How long a Rocket lasts (in ticks)
  uint16_t RocketTime;
  // Number of 'Greens' given to ships when they start
  uint16_t InitialBounty;
  // How likely a the ship is to take damamage (ie. lose a prize)
  // (0 = special - case-never, 1 = extremely likely, 5000 = almost never)
  uint16_t DamageFactor;
  // Maximum bounty that ships receive Team Prizes
  uint16_t PrizeShareLimit;
  // Bounty required by ships to attach as a turret
  uint16_t AttachBounty;
  // Time player has to carry soccer ball (in ticks)
  uint16_t SoccerBallThrowTimer;
  // Amount the friction on the soccer ball (how quickly it slows down
  // --higher numbers mean faster slowdown)
  uint16_t SoccerBallFriction;
  // How close the player must be in order to pick up ball (in pixels)
  uint16_t SoccerBallProximity;
  // Initial speed given to the ball when fired by the carrier
  uint16_t SoccerBallSpeed;
  // Number of turrets allowed on a ship
  uint8_t TurretLimit;
  // Number of bullets released when a 'Burst' is activated
  uint8_t BurstShrapnel;
  // Maximum number of mines allowed in ships
  uint8_t MaxMines;
  // Maximum number of Repels allowed in ships
  uint8_t RepelMax;
  // Maximum number of Bursts allowed in ships
  uint8_t BurstMax;
  // Maximum number of Decoys allowed in ships
  uint8_t DecoyMax;
  // Maximum number of Thor's Hammers allowed in ships
  uint8_t ThorMax;
  // Maximum number of Bricks allowed in ships
  uint8_t BrickMax;
  // Maximum number of Rockets allowed in ships
  uint8_t RocketMax;
  // Maximum number of Portals allowed in ships
  uint8_t PortalMax;
  // Initial number of Repels given to ships when they start
  uint8_t InitialRepel;
  // Initial number of Bursts given to ships when they start
  uint8_t InitialBurst;
  // Initial number of Bricks given to ships when they start
  uint8_t InitialBrick;
  // Initial number of Rockets given to ships when they start
  uint8_t InitialRocket;
  // Initial number of Rockets given to ships when they start
  uint8_t InitialThor;
  // Initial number of Decoys given to ships when they start
  uint8_t InitialDecoy;
  // Initial number of Portals given to ships when they start
  uint8_t InitialPortal;
  // Number of times a ship's bombs bounce before they explode on impact
  uint8_t BombBounceCount;

  // these values are not a bitfield they are interpreted as integer values
  struct {
      // 5 bits range from 0-31
    uint32_t ShrapnelMax : 5;
    uint32_t ShrapnelRate : 5;
    // 2 bits size allows values 0-3
    // 0 = not allowed, 1 = allowed, 2 = allowed/starts with
    uint32_t CloakStatus : 2;
    uint32_t StealthStatus : 2;
    uint32_t XRadarStatus : 2;
    uint32_t AntiWarpStatus : 2;
    // 0 = no guns, 1-3 = gun level
    uint32_t InitialGuns : 2;
    uint32_t MaxGuns : 2;
    uint32_t InitialBombs : 2;
    uint32_t MaxBombs : 2;
    // booleans
    // 0 = single, 1 = double
    uint32_t DoubleBarrel : 1;
    uint32_t EmpBomb : 1;
    uint32_t SeeMines : 1;
    uint32_t padding2 : 3;
  };
  unsigned char _padding[16];
};

// Structure to define the starting coordinates for teams 0-3
struct SpawnSettings {
  // X Coordinate for the center point where this team will start
  int X : 10;
  // Y Coordinate for the center point where this team will start
  int Y : 10;
  // How large of a circle from the center point this team can start
  int Radius : 9;
  int _pad : 3;

  MapCoord GetCoord() const {
    MapCoord result(X, Y);

    if (X < 0) {
      result.x = (uint16_t)-X;
    }

    if (Y < 0) {
      result.y = (uint16_t)-Y;
    }

    return result;
  }
};

// Likelihood of each prize appearing
struct PrizeWeightSettings {
  unsigned char QuickCharge;
  unsigned char Energy;
  unsigned char Rotation;
  unsigned char Stealth;
  unsigned char Cloak;
  unsigned char XRadar;
  unsigned char Warp;
  unsigned char Gun;
  unsigned char Bomb;
  unsigned char BouncingBullets;
  unsigned char Thruster;
  unsigned char TopSpeed;
  unsigned char Recharge;
  unsigned char Glue;  // Engine Shutdown
  unsigned char MultiFire;
  unsigned char Proximity;
  unsigned char AllWeapons;
  unsigned char Shields;
  unsigned char Shrapnel;
  unsigned char AntiWarp;
  unsigned char Repel;
  unsigned char Burst;
  unsigned char Decoy;
  unsigned char Thor;
  unsigned char MultiPrize;
  unsigned char Brick;
  unsigned char Rocket;
  unsigned char Portal;
};

class ClientSettings {
 public:

     float GetBulletAliveTime() const { return BulletAliveTime / 100.0f; }

     float GetBombAliveTime() const { return BombAliveTime / 100.0f; }

     float GetDecoyAliveTime() const { return DecoyAliveTime / 100.0f; }

     float GetMineAliveTime() const { return MineAliveTime / 100.0f; }

     float GetRepelTime() const { return RepelTime / 100.0f; }

     float GetBombExplodePixels() const { return BombExplodePixels / 16.0f; }

  struct {
    unsigned int Type : 8;
    // Whether to use exact bullet damage
    unsigned int ExactDamage : 1;
    // Whether to show dropped flags to spectators
    unsigned int HideFlags : 1;
    // Whether spectators are disallowed from having X radar
    unsigned int NoXRadar : 1;
    unsigned int SlowFrameRate : 3;
    // Whether to disable Continuum's screenshot feature
    unsigned int DisableScreenshot : 1;
    unsigned int _reserved : 1;
    unsigned int MaxTimerDrift : 3;
    // Whether to disable ball-passing through walls
    unsigned int DisableBallThroughWalls : 1;
    // Whether to disable ball killing in safe zones
    unsigned int DisableBallKilling : 1;
    unsigned int _padding : 11;
  };

  ShipSettings ShipSettings[8];

  // Maximum amount of damage that a L1 bullet will cause
  int BulletDamageLevel;
  // Amount of damage a bomb causes at its center point (for all bomb levels)
  int BombDamageLevel;

 private:
  // How long bullets live before disappearing (in ticks)
  int BulletAliveTime;
  // Time bomb is alive (in ticks)
  int BombAliveTime;
  // Time a decoy is alive (in ticks)
  int DecoyAliveTime;

 public:
  // Amount of time that can be spent in the safe zone (in ticks)
  int SafetyLimit;
  // Amount of random frequency shift applied to sounds in the game
  int FrequencyShift;
  // One more than the highest frequency allowed
  int MaxFrequency;
  // Speed at which players are repelled
  int RepelSpeed;
  // Time that mines are active (in ticks)

 private:
  int MineAliveTime;

 public:
  // Maximum amount of damage caused by a single burst bullet
  int BurstDamageLevel;
  // Amount of extra damage each bullet level will cause
  int BulletDamageUpgrade;
  // Time before flag is dropped by carrier (0=never)
  int FlagDropDelay;
  // Time a new player must wait before they are allowed to see flags
  int EnterGameFlaggingDelay;
  // Thrust value given while a rocket is active
  int RocketThrust;
  // Speed value given while a rocket is active
  int RocketSpeed;
  // Amount of damage shrapnel causes in its first 1/4 second of life
  int InactiveShrapDamage;
  // How often the wormhole switches its destination
  int WormholeSwitchTime;
  // Amount of time a ship is shutdown after application is reactivated
  int ActivateAppShutdownTime;
  // Speed that shrapnel travels
  int ShrapnelSpeed;

  SpawnSettings SpawnSettings[4];

  // Percentage of the ping time that is spent on the C2S portion of the
  // ping(used in more accurately syncronizing clocks)
  short SendRoutePercent;
  // How long after the proximity sensor is triggered before bomb explodes
  short BombExplodeDelay;
  // Amount of time between position packets sent by client
  short SendPositionDelay;
  // Blast radius in pixels for an L1 bomb (L2 bombs double this, L3 bombs
  // triple this)

 private:
  short BombExplodePixels;
  // How long the prize exists that appears after killing somebody

 public:
  short DeathPrizeTime;
  // How long the screen jitters from a bomb hit (in ticks)
  short JitterTime;
  // How long after a player dies before he can re-enter the game (in ticks)
  short EnterDelay;
  // Time the player is affected by an 'Engine Shutdown' Prize (in ticks)
  short EngineShutdownTime;
  // Radius of proximity trigger in tiles (each bomb level adds 1 to this
  // amount)
  short ProximityDistance;
  // Number of points added to players bounty each time he kills an opponent
  short BountyIncreaseForKill;
  // How bouncy the walls are (16 = no speed loss)
  short BounceFactor;
  // A number representing how much the map is zoomed out for radar.
  // (48 = whole map on radar, 49 + = effectively disable radar)
  short MapZoomFactor;
  //
  short MaxBonus;
  //
  short MaxPenalty;
  //
  short RewardBase;

 private:
  // Time players are affected by the repel (in ticks)
  short RepelTime;

 public:
  // Number of pixels from the player that are affected by a repel
  short RepelDistance;
  // Amount of time between ticker help messages
  short TickerDelay;
  // Whether the flaggers appear on radar in red
  short FlaggerOnRadar;
  // Number of times more points are given to a flagger (1 = double points, 2 =
  // triple points)
  short FlaggerKillMultiplier;
  // Number of prizes hidden is based on number of players in game.
  // This number adjusts the formula, higher numbers mean more prizes.
  // (Note: 10000 is max, 10 greens per person)
  short PrizeFactor;
  // How often prizes are regenerated (in ticks)
  short PrizeDelay;
  // Distance from center of arena that prizes/flags/soccer-balls will spawn
  short MinimumVirtual;
  // Amount of additional distance added to MinimumVirtual for each player that
  // is in the game
  short UpgradeVirtual;
  // Maximum amount of time that a hidden prize will remain on screen. (actual
  // time is random)
  short PrizeMaxExist;
  // Minimum amount of time that a hidden prize will remain on screen. (actual
  // time is random)
  short PrizeMinExist;
  //  Odds of getting a negative prize.  (1 = every prize, 32000 = extremely
  //  rare)
  short PrizeNegativeFactor;
  // How often doors attempt to switch their state
  short DoorDelay;
  // Distance Anti-Warp affects other players (in pixels) (note: enemy must also
  // be on radar)
  short AntiWarpPixels;
  // Door mode (-2=all doors completely random, -1=weighted random
  // (some doors open more often than others), 0 - 255 = fixed doors (1
  // bit of byte for each door specifying whether it is open or not)
  short DoorMode;
  // Amount of time that a user can get no data from server before flags are
  // hidden from view for 10 seconds
  short FlagBlankDelay;
  // Amount of time that a user can get no data from server before flags he is
  // carrying are dropped
  short NoDataFlagDropDelay;
  // Number of random greens given with a MultiPrize
  short MultiPrizeCount;
  // How long bricks last (in ticks)
  short BrickTime;
  // When ships are randomly placed in the arena, this parameter will
  // limit how far from the center of the arena they can be placed
  // (1024 = anywhere)
  short WarpRadiusLimit;
  // Maximum time recharge is stopped on players hit with an EMP bomb
  short EBombShutdownTime;
  // Percentage of normal damage applied to an EMP bomb (in 0.1%)
  short EBombDamagePercent;
  // Size of area between blinded radar zones (in pixels)
  short RadarNeutralSize;
  // How long a portal is active
  short WarpPointDelay;
  // Amount of energy that constitutes a near-death experience (ships
  // bounty will be decreased by 1 when this occurs -- used for dueling zone)
  short NearDeathLevel;
  // Percentage of normal damage applied to a bouncing bomb (in 0.1%)
  short BBombDamagePercent;
  // Percentage of normal damage applied to shrapnel (relative to bullets of
  // same level) (in 0.1%)
  short ShrapnelDamagePercent;
  // Amount of latency S2C that constitutes a slow packet
  short ClientSlowPacketTime;
  // Minimum kill reward that a player must get in order to have his flag drop
  // timer reset
  short FlagDropResetReward;
  // Percentage of normal weapon firing cost for flaggers (in 0.1%)
  short FlaggerFireCostPercent;
  // Percentage of normal damage received by flaggers (in 0.1%)
  short FlaggerDamagePercent;
  // Delay given to flaggers for firing bombs (zero is ships normal firing rate)
  // (do not set this number less than 20)
  short FlaggerBombFireDelay;
  // How long after the ball is fired before anybody can pick it up (in ticks)
  short PassDelay;
  // Amount of time a player can receive no data from server and still pick up
  // the soccer ball
  short BallBlankDelay;
  // Amount of time a user can receive no data from server before connection is
  // terminated
  short S2CNoDataKickoutDelay;
  // Amount of thrust adjustment player carrying flag gets (negative numbers
  // mean less thrust)
  short FlaggerThrustAdjustment;
  // Amount of speed adjustment player carrying flag gets (negative numbers mean
  // slower)
  short FlaggerSpeedAdjustment;
  // Number of packets to sample S2C before checking for kickout
  short ClientSlowPacketSampleSize;
  short _UnusedShort[5];

  // Whether shrapnel spreads in circular or random patterns
  char ShrapnelRandom;
  // Whether the ball bounces off walls
  char BallBounce;
  // Whether the ball carrier can fire his bombs
  char AllowBombs;
  // Whether the ball carrier can fire his guns
  char AllowGuns;
  // Goal configuration ($GOAL_ALL, $GOAL_LEFTRIGHT, $GOAL_TOPBOTTOM,
  // $GOAL_CORNERS_3_1, $GOAL_CORNERS_1_3, $GOAL_SIDES_3_1,
  // $GOAL_SIDES_1_3)
  char SoccerMode;
  // Maximum number of people on a public team
  char MaxPerTeam;
  // Maximum number of people on a private team
  char MaxPerPrivateTeam;
  // Maximum number of mines allowed to be placed by an entire team
  char TeamMaxMines;
  // Whether a wormhole affects bombs
  char GravityBombs;
  // Whether proximity bombs have a firing safety.  If enemy ship is
  // within proximity radius, will it allow you to fire
  char BombSafety;
  //
  char MessageReliable;
  // Whether prize packets are sent reliably (C2S)
  char TakePrizeReliable;
  // Whether players can send audio messages
  char AllowAudioMessages;
  // Number of prizes that are regenerated every PrizeDelay
  char PrizeHideCount;
  // Whether regular players receive sysop data about a ship
  char ExtraPositionData;
  // Whether to check for slow frames on the client (possible cheat
  // technique) (flawed on some machines, do not use)
  char SlowFrameCheck;
  // Whether the flags can be picked up and carried
  // (0=no, 1=yes, 2 = yes - one at a time, 3 = yes - two at a time, 4 = three,
  // etc..)
  char CarryFlags;
  // Whether saved ships are allowed (do not allow saved ship in zones where sub
  // - arenas may have differing parameters)
  char AllowSavedShips;
  // Radar mode (0=normal, 1=half/half, 2=quarters, 3=half/half-see team mates,
  // 4 = quarters - see team mates)
  char RadarMode;
  // Whether the zone plays victory music or not
  char VictoryMusic;
  // Whether the flaggers get a gun upgrade
  char FlaggerGunUpgrade;
  // Whether the flaggers get a bomb upgrade
  char FlaggerBombUpgrade;
  // If player with soccer ball should use the Flag:Flagger* ship adjustments or
  // not
  char UseFlagger;
  // Whether the balls location is displayed at all times or not
  char BallLocation;
  // How many ticks to activate a fake antiwarp after attaching, portaling, or
  // warping
  char AntiwarpSettleDelay;
  char _UnusedByte[7];

  PrizeWeightSettings PrizeWeights;
};
#pragma pack(pop)

}  // namespace marvin
