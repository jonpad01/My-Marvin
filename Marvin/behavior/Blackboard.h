#pragma once

#include <any>
#include <optional>
#include <string>
#include <unordered_map>

#include "../player.h"


namespace marvin {
namespace behavior {

    class Blackboard {
    public:
        Blackboard() : chatcount_(0), chatlock_(false), freqlock_(false), ship_(0), freq_(0), target_(nullptr) {}




        bool Has(const std::string& key) { return data_.find(key) != data_.end(); }

        template <typename T>
        void Set(const std::string& key, const T& value) {
            data_[key] = std::any(value);
        }

        template <typename T>
        std::optional<T> Value(const std::string& key) {
            auto iter = data_.find(key);

            if (iter == data_.end()) {
                return std::nullopt;
            }

            auto& any = iter->second;

            try {
                return std::any_cast<T>(any);
            }
            catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }

        template <typename T>
        T ValueOr(const std::string& key, const T& or_result) {
            return Value<T>(key).value_or(or_result);
        }

        void Clear() { data_.clear(); }
        void Erase(const std::string& key) { data_.erase(key); }



        void SetChatCount(std::size_t count) { chatcount_ = count; }
        std::size_t GetChatCount() { return chatcount_; }

        void SetChatLock(bool lock) { chatlock_ = lock; }
        bool GetChatLock() { return chatlock_; }

        void SetFreqLock(bool lock) { freqlock_ = lock; }
        bool GetFreqLock() { return freqlock_; }

        void SetShip(int ship) { ship_ = ship; }
        int GetShip() { return ship_; }

        void SetFreq(int freq) { freq_ = freq; }
        int GetFreq() { return freq_; }

        void SetTarget(const Player* target) { target_ = target; }
        const Player* GetTarget() { return target_; }







    private:

        std::size_t chatcount_;
        bool chatlock_;
        bool freqlock_;
        int ship_;
        int freq_;

        const Player* target_;



        std::unordered_map<std::string, std::any> data_;
    };

}  // namespace behavior
}  // namespace marvin
