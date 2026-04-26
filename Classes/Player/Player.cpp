#include "Player.h"

USING_NS_CC;

namespace
{
constexpr float kMaxDamageReduction = 0.8f;
constexpr float kMinAttackCooldown = 0.06f;
}

Player* Player::create(const Config& config)
{
    auto* player = new (std::nothrow) Player();
    if (player && player->init(config))
    {
        player->autorelease();
        return player;
    }

    CC_SAFE_DELETE(player);
    return nullptr;
}

bool Player::init(const Config& config)
{
    if (!Node::init())
    {
        return false;
    }

    _config = config;
    _stats.currentHP = _config.maxHP;
    _stats.currentMana = _config.maxMana;

    recalculateDerivedStats();
    setupVisuals();
    setupInput();

    scheduleUpdate();
    return true;
}

void Player::update(float dt)
{
    if (!isAlive())
    {
        return;
    }

    if (_iFrameTimer > 0.0f)
    {
        _iFrameTimer -= dt;
    }

    processMovement(dt);
    processAttacks(dt);

    if (_debugAim != nullptr)
    {
        _debugAim->clear();
        _debugAim->drawLine(Vec2::ZERO, _aimDirection * 28.0f, Color4F(0.36f, 0.89f, 1.0f, 0.8f));
    }
}

void Player::grantXP(int amount)
{
    if (amount <= 0 || !isAlive())
    {
        return;
    }

    _stats.xp += amount;
    tryLevelUp();
}

void Player::grantBoostCredits(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    _stats.boostCredits += amount;
}

void Player::consumeMana(float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    _stats.currentMana = std::max(0.0f, _stats.currentMana - amount);
    if (_manaChangedCallback)
    {
        _manaChangedCallback(_stats.currentMana, _config.maxMana);
    }
}

void Player::restoreMana(float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    _stats.currentMana = std::min(_config.maxMana, _stats.currentMana + amount);
    if (_manaChangedCallback)
    {
        _manaChangedCallback(_stats.currentMana, _config.maxMana);
    }
}

void Player::applyDamage(float damage)
{
    if (!isAlive() || damage <= 0.0f || _iFrameTimer > 0.0f)
    {
        return;
    }

    const float reducedDamage = damage * (1.0f - _damageReduction);
    _stats.currentHP = std::max(0.0f, _stats.currentHP - reducedDamage);
    _iFrameTimer = _config.invulnerabilityWindow;

    if (_damagedCallback)
    {
        _damagedCallback(_stats.currentHP, _config.maxHP);
    }

    if (!isAlive())
    {
        stopAllActions();
        runAction(Blink::create(0.45f, 5));
        return;
    }

    runAction(Sequence::create(TintTo::create(0.06f, 255, 75, 75), TintTo::create(0.08f, 255, 255, 255), nullptr));
}

void Player::heal(float amount)
{
    if (amount <= 0.0f || !isAlive())
    {
        return;
    }

    _stats.currentHP = std::min(_config.maxHP, _stats.currentHP + amount);
    if (_damagedCallback)
    {
        _damagedCallback(_stats.currentHP, _config.maxHP);
    }
}

bool Player::isAlive() const
{
    return _stats.currentHP > 0.0f;
}

float Player::getMoveSpeed() const
{
    return _config.moveSpeed * _moveSpeedMultiplier;
}

float Player::getAttackCooldown() const
{
    return _currentAttackCooldown;
}

const Player::RuntimeStats& Player::getStats() const
{
    return _stats;
}

const Player::Config& Player::getConfig() const
{
    return _config;
}

void Player::addMoveSpeedMultiplier(float addititveMultiplier)
{
    _moveSpeedMultiplier = std::max(0.2f, _moveSpeedMultiplier + addititveMultiplier);
}

void Player::addAttackSpeedMultiplier(float additiveMultiplier)
{
    _attackSpeedMultiplier = std::max(0.2f, _attackSpeedMultiplier + additiveMultiplier);
    recalculateDerivedStats();
}

void Player::addMaxHP(float amount, bool alsoHeal)
{
    if (amount <= 0.0f)
    {
        return;
    }

    _config.maxHP += amount;
    if (alsoHeal)
    {
        _stats.currentHP += amount;
    }

    _stats.currentHP = std::min(_stats.currentHP, _config.maxHP);
    if (_damagedCallback)
    {
        _damagedCallback(_stats.currentHP, _config.maxHP);
    }
}

void Player::addMaxMana(float amount, bool alsoRestore)
{
    if (amount <= 0.0f)
    {
        return;
    }

    _config.maxMana += amount;
    if (alsoRestore)
    {
        _stats.currentMana += amount;
    }

    _stats.currentMana = std::min(_stats.currentMana, _config.maxMana);
    if (_manaChangedCallback)
    {
        _manaChangedCallback(_stats.currentMana, _config.maxMana);
    }
}

void Player::addDamageReduction(float additiveAmount)
{
    _damageReduction = clampf(_damageReduction + additiveAmount, 0.0f, kMaxDamageReduction);
}

void Player::setAttackRequestCallback(AttackRequestCallback callback)
{
    _attackRequestCallback = std::move(callback);
}

void Player::setOnDamagedCallback(DamagedCallback callback)
{
    _damagedCallback = std::move(callback);
}

void Player::setOnManaChangedCallback(ManaChangedCallback callback)
{
    _manaChangedCallback = std::move(callback);
}

void Player::setOnLevelUpCallback(LevelUpCallback callback)
{
    _levelUpCallback = std::move(callback);
}

void Player::setupVisuals()
{
    _bodySprite = Sprite::create("player/moderator_avatar.png");
    if (_bodySprite == nullptr)
    {
        _bodySprite = Sprite::create();
        _bodySprite->setTextureRect(Rect(0.0f, 0.0f, 30.0f, 30.0f));
        _bodySprite->setColor(Color3B(88, 101, 242));
    }
    addChild(_bodySprite);

    auto* glow = DrawNode::create();
    glow->drawSolidCircle(Vec2::ZERO, 22.0f, 0.0f, 36, Color4F(0.37f, 0.24f, 1.0f, 0.22f));
    addChild(glow, -1);

    _debugAim = DrawNode::create();
    addChild(_debugAim, 2);
}

void Player::setupInput()
{
    _keyboardListener = EventListenerKeyboard::create();
    _keyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event*) {
        _keyDown[keyCode] = true;
    };

    _keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event*) {
        _keyDown[keyCode] = false;
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(_keyboardListener, this);

    _mouseListener = EventListenerMouse::create();
    _mouseListener->onMouseMove = [this](Event* event) {
        auto* mouseEvent = static_cast<EventMouse*>(event);
        _cachedMouseWorld = _director->convertToGL(Vec2(mouseEvent->getCursorX(), mouseEvent->getCursorY()));
        updateAimDirectionFromMouse(_cachedMouseWorld);
    };

    _mouseListener->onMouseDown = [this](Event* event) {
        auto* mouseEvent = static_cast<EventMouse*>(event);
        if (mouseEvent->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            _isPrimaryFireHeld = true;
            _attackTimer = 0.0f;
        }
    };

    _mouseListener->onMouseUp = [this](Event* event) {
        auto* mouseEvent = static_cast<EventMouse*>(event);
        if (mouseEvent->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            _isPrimaryFireHeld = false;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(_mouseListener, this);
}

void Player::processMovement(float dt)
{
    _moveInput = Vec2::ZERO;

    if (_keyDown[EventKeyboard::KeyCode::KEY_A] || _keyDown[EventKeyboard::KeyCode::KEY_LEFT_ARROW])
    {
        _moveInput.x -= 1.0f;
    }
    if (_keyDown[EventKeyboard::KeyCode::KEY_D] || _keyDown[EventKeyboard::KeyCode::KEY_RIGHT_ARROW])
    {
        _moveInput.x += 1.0f;
    }
    if (_keyDown[EventKeyboard::KeyCode::KEY_W] || _keyDown[EventKeyboard::KeyCode::KEY_UP_ARROW])
    {
        _moveInput.y += 1.0f;
    }
    if (_keyDown[EventKeyboard::KeyCode::KEY_S] || _keyDown[EventKeyboard::KeyCode::KEY_DOWN_ARROW])
    {
        _moveInput.y -= 1.0f;
    }

    if (!_moveInput.isZero())
    {
        _moveInput.normalize();
        setPosition(getPosition() + (_moveInput * getMoveSpeed() * dt));
    }
}

void Player::processAttacks(float dt)
{
    _attackTimer += dt;

    if (!_isPrimaryFireHeld || _attackRequestCallback == nullptr)
    {
        return;
    }

    if (_attackTimer < _currentAttackCooldown)
    {
        return;
    }

    _attackTimer = 0.0f;
    _attackRequestCallback(getPosition(), _aimDirection);
}

void Player::updateAimDirectionFromMouse(const Vec2& worldMouse)
{
    const Vec2 delta = worldMouse - getPosition();
    if (delta.lengthSquared() <= 0.001f)
    {
        return;
    }

    _aimDirection = delta.getNormalized();
}

void Player::recalculateDerivedStats()
{
    _currentAttackCooldown = std::max(kMinAttackCooldown, _config.baseAttackCooldown / _attackSpeedMultiplier);
}

void Player::tryLevelUp()
{
    while (_stats.xp >= _stats.xpToNextLevel)
    {
        _stats.xp -= _stats.xpToNextLevel;
        _stats.level += 1;
        _stats.xpToNextLevel = computeXPForLevel(_stats.level);

        if (_levelUpCallback)
        {
            _levelUpCallback(_stats.level);
        }
    }
}

int Player::computeXPForLevel(int targetLevel) const
{
    const int baseXP = 35;
    const int scale = 14;
    return baseXP + ((targetLevel - 1) * scale);
}
