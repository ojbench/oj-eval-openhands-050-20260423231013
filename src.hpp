
#pragma once

#include <array>
#include <exception>
#include <optional>
#include <vector>
#include <deque>

class InvalidOperation : public std::exception {
public:
    const char* what() const noexcept override {
        return "invalid operation";
    }
};

struct PlayInfo {
    int dummyCount = 0;
    int magnifierCount = 0;
    int converterCount = 0;
    int cageCount = 0;
};

class GameState {
public:
    enum class BulletType { Live, Blank };
    enum class ItemType { Dummy, Magnifier, Converter, Cage };

    GameState() {
        // Initialize game state
        currentPlayer = 0;
        playerHP = {5, 5};
        playerItems = {PlayInfo{}, PlayInfo{}};
        usedCageThisTurn = false;
        cageActive = false;
        magnifierRevealed = std::nullopt;
    }

    void fireAtOpponent(BulletType topBulletBeforeAction) {
        // Consume the bullet
        consumeBullet(topBulletBeforeAction);
        
        // Apply damage if live bullet
        if (topBulletBeforeAction == BulletType::Live) {
            playerHP[1 - currentPlayer]--;
        }
        
        // End turn (unless cage prevents it)
        endTurn();
    }

    void fireAtSelf(BulletType topBulletBeforeAction) {
        // Consume the bullet
        consumeBullet(topBulletBeforeAction);
        
        // Apply damage if live bullet
        if (topBulletBeforeAction == BulletType::Live) {
            playerHP[currentPlayer]--;
        }
        
        // End turn only if bullet was live (unless cage prevents it)
        if (topBulletBeforeAction == BulletType::Live) {
            endTurn();
        }
    }

    void useDummy(BulletType topBulletBeforeUse) {
        // Check if player has dummy
        if (playerItems[currentPlayer].dummyCount <= 0) {
            throw InvalidOperation();
        }
        
        // Consume dummy
        playerItems[currentPlayer].dummyCount--;
        
        // Consume the bullet
        consumeBullet(topBulletBeforeUse);
        
        // Does not end turn
    }

    void useMagnifier(BulletType topBulletBeforeUse) {
        // Check if player has magnifier
        if (playerItems[currentPlayer].magnifierCount <= 0) {
            throw InvalidOperation();
        }
        
        // Consume magnifier
        playerItems[currentPlayer].magnifierCount--;
        
        // Reveal next bullet type
        magnifierRevealed = topBulletBeforeUse;
        
        // Does not end turn
    }

    void useConverter(BulletType topBulletBeforeUse) {
        // Check if player has converter
        if (playerItems[currentPlayer].converterCount <= 0) {
            throw InvalidOperation();
        }
        
        // Consume converter
        playerItems[currentPlayer].converterCount--;
        
        // Reveal and flip next bullet type
        BulletType flippedType = (topBulletBeforeUse == BulletType::Live) ? BulletType::Blank : BulletType::Live;
        magnifierRevealed = flippedType;
        
        // Update bullet counts
        if (topBulletBeforeUse == BulletType::Live) {
            liveCount--;
            blankCount++;
        } else {
            blankCount--;
            liveCount++;
        }
        
        // Does not end turn
    }

    void useCage() {
        // Check if player has cage
        if (playerItems[currentPlayer].cageCount <= 0) {
            throw InvalidOperation();
        }
        
        // Check if already used cage this turn
        if (usedCageThisTurn) {
            throw InvalidOperation();
        }
        
        // Consume cage
        playerItems[currentPlayer].cageCount--;
        
        // Mark cage as used this turn and activate cage effect
        usedCageThisTurn = true;
        cageActive = true;
        
        // Does not end turn
    }

    void reloadBullets(int liveCount, int blankCount) {
        this->liveCount = liveCount;
        this->blankCount = blankCount;
        magnifierRevealed = std::nullopt;
    }

    void reloadItem(int playerId, ItemType item) {
        if (playerId != 0 && playerId != 1) {
            throw InvalidOperation();
        }
        
        switch (item) {
            case ItemType::Dummy:
                playerItems[playerId].dummyCount++;
                break;
            case ItemType::Magnifier:
                playerItems[playerId].magnifierCount++;
                break;
            case ItemType::Converter:
                playerItems[playerId].converterCount++;
                break;
            case ItemType::Cage:
                playerItems[playerId].cageCount++;
                break;
        }
    }

    double nextLiveBulletProbability() const {
        if (magnifierRevealed.has_value()) {
            return (magnifierRevealed.value() == BulletType::Live) ? 1.0 : 0.0;
        }
        
        int total = liveCount + blankCount;
        if (total == 0) return 0.0;
        return static_cast<double>(liveCount) / total;
    }

    double nextBlankBulletProbability() const {
        if (magnifierRevealed.has_value()) {
            return (magnifierRevealed.value() == BulletType::Blank) ? 1.0 : 0.0;
        }
        
        int total = liveCount + blankCount;
        if (total == 0) return 0.0;
        return static_cast<double>(blankCount) / total;
    }

    int winnerId() const {
        if (playerHP[0] <= 0) return 1;
        if (playerHP[1] <= 0) return 0;
        return -1;
    }

private:
    int currentPlayer;  // 0 or 1
    std::array<int, 2> playerHP;  // HP of both players
    std::array<PlayInfo, 2> playerItems;  // Items of both players
    int liveCount = 0;
    int blankCount = 0;
    bool usedCageThisTurn = false;  // Whether cage has been used this turn
    bool cageActive = false;  // Whether cage effect is active
    std::optional<BulletType> magnifierRevealed;  // Next bullet type if revealed by magnifier
    
    void consumeBullet(BulletType bulletType) {
        // Update bullet counts
        if (bulletType == BulletType::Live) {
            liveCount--;
        } else {
            blankCount--;
        }
        
        // Clear magnifier revelation since bullet is consumed
        magnifierRevealed = std::nullopt;
    }
    
    void endTurn() {
        // Check if cage prevents turn end
        if (cageActive) {
            cageActive = false;
            return;  // Continue current player's turn
        }
        
        // Switch to other player
        currentPlayer = 1 - currentPlayer;
        
        // Reset cage usage tracking for new turn
        usedCageThisTurn = false;
    }
};
