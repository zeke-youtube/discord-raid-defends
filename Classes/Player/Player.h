#pragma once

#include "cocos2d.h"
#include <functional>
#include <unordered_map>

class Player final : public cocos2d::Node
{
public:
    struct Config
    {
        float maxHP = 100.0f;                 // Server Reputation
        float maxMana = 100.0f;               // Mod Sanity
        float moveSpeed = 240.0f;
        float baseAttackCooldown = 0.38f;
        float invulnerabilityWindow = 0.35f;
        float xpPickupRadius = 88.0f;
    };

    struct RuntimeStats
    {
        float currentHP = 100.0f;
        float currentMana = 100.0f;
        int level = 1;
        int xp = 0;
        int xpToNextLevel = 35;
        int boostCredits = 0;
    };

    using AttackRequestCallback = std::function<void(const cocos2d::Vec2& origin, const cocos2d::Vec2& aimDirection)>;
    using DamagedCallback = std::function<void(float currentHP, float maxHP)>;
    using ManaChangedCallback = std::function<void(float currentMana, float maxMana)>;
    using LevelUpCallback = std::function<void(int newLevel)>;

    static Player* create(const Config& config = Config());

    bool init(const Config& config);

    void update(float dt) override;

    // Gameplay API
    void grantXP(int amount);
    void grantBoostCredits(int amount);
    void consumeMana(float amount);
    void restoreMana(float amount);
    void applyDamage(float damage);
    void heal(float amount);
    bool isAlive() const;

    // Derived values and accessors
    float getMoveSpeed() const;
    float getAttackCooldown() const;
    const RuntimeStats& getStats() const;
    const Config& getConfig() const;

    // Upgrade modifiers
    void addMoveSpeedMultiplier(float addititveMultiplier);
    void addAttackSpeedMultiplier(float additiveMultiplier);
    void addMaxHP(float amount, bool alsoHeal = true);
    void addMaxMana(float amount, bool alsoRestore = true);
    void addDamageReduction(float additiveAmount);

    // Combat hooks
    void setAttackRequestCallback(AttackRequestCallback callback);

    // UI hooks
    void setOnDamagedCallback(DamagedCallback callback);
    void setOnManaChangedCallback(ManaChangedCallback callback);
    void setOnLevelUpCallback(LevelUpCallback callback);

private:
    Player() = default;
    ~Player() override = default;

    void setupVisuals();
    void setupInput();
    void processMovement(float dt);
    void processAttacks(float dt);
    void updateAimDirectionFromMouse(const cocos2d::Vec2& worldMouse);
    void recalculateDerivedStats();
    void tryLevelUp();
    int computeXPForLevel(int targetLevel) const;

    cocos2d::Sprite* _bodySprite = nullptr;
    cocos2d::DrawNode* _debugAim = nullptr;
    cocos2d::EventListenerKeyboard* _keyboardListener = nullptr;
    cocos2d::EventListenerMouse* _mouseListener = nullptr;

    Config _config;
    RuntimeStats _stats;

    std::unordered_map<cocos2d::EventKeyboard::KeyCode, bool> _keyDown;
    cocos2d::Vec2 _moveInput = cocos2d::Vec2::ZERO;
    cocos2d::Vec2 _aimDirection = cocos2d::Vec2::UNIT_Y;
    cocos2d::Vec2 _cachedMouseWorld = cocos2d::Vec2::ZERO;

    float _attackSpeedMultiplier = 1.0f;
    float _moveSpeedMultiplier = 1.0f;
    float _damageReduction = 0.0f;

    float _currentAttackCooldown = 0.0f;
    float _attackTimer = 0.0f;
    float _iFrameTimer = 0.0f;

    bool _isPrimaryFireHeld = false;

    AttackRequestCallback _attackRequestCallback;
    DamagedCallback _damagedCallback;
    ManaChangedCallback _manaChangedCallback;
    LevelUpCallback _levelUpCallback;
};
